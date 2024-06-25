/*
 * RedRenderVideoHal.cpp
 *
 *  Created on: 2022年8月19日
 *      Author: liuhongda
 */

#include "RedRenderVideoHal.h"
#include "RedMsg.h"
#include "RedProp.h"
#include "base/RedConfig.h"
#include <cfloat>

#define TAG "RedRenderVideoHal"

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.15
#define MAX_FRAME_DURATION 10.0
#define REFRESH_RATE 0.01

REDPLAYER_NS_BEGIN;

CRedRenderVideoHal::CRedRenderVideoHal(int id, sp<CVideoProcesser> &processer,
                                       const sp<VideoState> &state,
                                       NotifyCallback notify_cb)
    : mID(id), mVideoProcesser(processer), mVideoState(state),
      mNotifyCb(notify_cb) {}

CRedRenderVideoHal::~CRedRenderVideoHal() {
  mVideoRender.reset();
  mVideoProcesser.reset();
  mMetaData.reset();
  sws_freeContext(mSwsContext);
}

RED_ERR CRedRenderVideoHal::Prepare(sp<MetaData> &metadata) {
  RED_ERR ret = OK;
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      return OK;
    }
  }
  mMetaData = metadata;

  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      return OK;
    }
    this->run();
  }
  return ret;
}

#if defined(__ANDROID__) || defined(__HARMONY__)
RED_ERR
CRedRenderVideoHal::setVideoSurface(const sp<RedNativeWindow> &surface) {
  std::unique_lock<std::mutex> lck(mSurfaceLock);
  if (!surface) {
    return ME_ERROR;
  }
  if (!surface->get()) {
    AV_LOGI_ID(TAG, mID, "set surface null .\n");
  }

  if (!mNativeWindow || mNativeWindow->get() != surface->get()) {
    AV_LOGD_ID(TAG, mID, "surface update to %p\n", surface->get());
    mSurfaceUpdate = true;
    mNativeWindow = surface;
    if (mPaused &&
        mClusterType == RedRender::VRClusterType::VRClusterTypeOpenGL) {
      mForceRefresh = true;
    }
    mSurfaceSet.notify_one();
  }

  return OK;
}
#endif

#if defined(__APPLE__)
UIView *CRedRenderVideoHal::initWithFrame(int type, CGRect cgrect) {
  std::unique_lock<std::mutex> lck(mLock);

  std::unique_ptr<RedRender::VideoRendererFactory> videoRendererFactory =
      std::make_unique<RedRender::VideoRendererFactory>();
  if (!videoRendererFactory) {
    AV_LOGE_ID(TAG, mID, "[%s:%d] VideoRendererFactory construct error .\n",
               __FUNCTION__, __LINE__);
    return nullptr;
  }

  switch (type) {
  case RENDER_TYPE_OPENGL: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeOpenGL);
    mClusterType = RedRender::VRClusterTypeOpenGL;
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
    break;
  }
  case RENDER_TYPE_METAL: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeMetal);
    mClusterType = RedRender::VRClusterTypeMetal;
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
    break;
  }
  case RENDER_TYPE_AVSBDL: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeAVSBDL);
    mClusterType = RedRender::VRClusterTypeAVSBDL;
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
    break;
  }
  default:
    AV_LOGE_ID(TAG, mID, "[%s:%d] unknown render type %d .\n", __FUNCTION__,
               __LINE__, type);
    break;
  }

  videoRendererFactory.reset();

  if (!mVideoRender) {
    AV_LOGE_ID(TAG, mID, "%s mVideoRender create error\n", __func__);
    return nullptr;
  }

  RedRender::VRError ret = mVideoRender->init();
  if (ret != RedRender::VRErrorNone) {
    lck.unlock();
    notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY,
                   static_cast<int32_t>(ret));
    AV_LOGE_ID(TAG, mID, "%s mVideoRender init error %d\n", __func__,
               static_cast<int>(ret));
    return nullptr;
  }

  ret = mVideoRender->initWithFrame(cgrect);
  if (ret != RedRender::VRErrorNone) {
    lck.unlock();
    notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY,
                   static_cast<int32_t>(ret));
    AV_LOGE_ID(TAG, mID, "%s mVideoRender initWithFrame error %d\n", __func__,
               static_cast<int>(ret));
    return nullptr;
  }
  mRenderSetuped = true;
  return mVideoRender->getRedRenderView();
}
#endif

double CRedRenderVideoHal::ComputeDelay(double delay) {
  double diff = 0.0;
  if (!mVideoState->audio_clock || !mVideoState->video_clock)
    return delay;
  if (getMasterSyncType(mVideoState) != CLOCK_VIDEO) {
    diff = mVideoState->video_clock->GetClock() - getMasterClock(mVideoState);
    double sync_threshold =
        std::max(AV_SYNC_THRESHOLD_MIN, std::min(AV_SYNC_THRESHOLD_MAX, delay));
    if (std::abs(diff) < AV_NOSYNC_THRESHOLD) {
      if (diff <= -sync_threshold) {
        delay = std::max(0.0, delay + diff);
      } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
        delay = delay + diff;
      } else if (diff >= sync_threshold) {
        delay *= 2;
      }
    }
  }
  delay /= mVideoState->playback_rate;
  mVideoState->stat.avdiff = diff;
  mVideoState->stat.avdelay = delay;
  return delay;
}

double
CRedRenderVideoHal::ComputeDuration(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (buffer->serial == mFrameTick.serial) {
    double duration = buffer->pts / 1000.0 - mFrameTick.pts;
    if (duration <= FLT_EPSILON || duration > MAX_FRAME_DURATION) {
      return mFrameTick.duration;
    } else {
      return duration;
    }
  } else {
    return 0.0;
  }
}

RED_ERR CRedRenderVideoHal::ReadFrame(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (!mVideoProcesser)
    return ME_ERROR;
  return mVideoProcesser->getFrame(buffer);
}

RED_ERR CRedRenderVideoHal::Init() {
  std::unique_lock<std::mutex> lck(mLock);

  if (mRenderSetuped) {
    return OK;
  }

  switch (mVideoState->stat.vdec_type) {
  case RED_PROPV_DECODER_AVCODEC:
    mClusterType = RedRender::VRClusterTypeOpenGL;
    break;
  case RED_PROPV_DECODER_MEDIACODEC:
    mClusterType = RedRender::VRClusterTypeMediaCodec;
    break;
  case RED_PROPV_DECODER_VIDEOTOOLBOX:
    mClusterType = RedRender::VRClusterTypeMetal;
    break;
  case RED_PROPV_DECODER_HARMONY_VIDEO_DECODER:
    mClusterType = RedRender::VRClusterTypeHarmonyVideoDecoder;
    break;
  default:
    AV_LOGE_ID(TAG, mID, "unknown vdec type %" PRId64 ".\n",
               mVideoState->stat.vdec_type);
    return NO_INIT;
  }

  std::unique_ptr<RedRender::VideoRendererFactory> videoRendererFactory =
      std::make_unique<RedRender::VideoRendererFactory>();
  if (!videoRendererFactory) {
    AV_LOGE_ID(TAG, mID, "[%s:%d] VideoRendererFactory construct error .\n",
               __FUNCTION__, __LINE__);
    return NO_INIT;
  }

  switch (mClusterType) {
  case RedRender::VRClusterTypeOpenGL: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeOpenGL);
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
  } break;
  case RedRender::VRClusterTypeMetal: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeMetal);
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
  } break;
  case RedRender::VRClusterTypeMediaCodec: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeMediaCodec);
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
  } break;
  case RedRender::VRClusterTypeHarmonyVideoDecoder: {
    RedRender::VideoRendererInfo videRendererInfo(
        RedRender::VRClusterTypeHarmonyVideoDecoder);
    mVideoRender =
        videoRendererFactory->createVideoRenderer(videRendererInfo, mID);
  } break;
  case RedRender::VRClusterTypeUnknown:
  default:
    AV_LOGE_ID(TAG, mID, "[%s:%d] VideoRendererClusterTypeUnknownWrapper_ .\n",
               __FUNCTION__, __LINE__);
    break;
  }

  if (!mVideoRender) {
    AV_LOGE_ID(TAG, mID, "mVideoRender create error");
    return NO_INIT;
  }

  RedRender::VRError ret = mVideoRender->init();
  if (ret != RedRender::VRErrorNone) {
    lck.unlock();
    notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY,
                   static_cast<int32_t>(ret));
    AV_LOGE_ID(TAG, mID, "mVideoRender init error");
    return NO_INIT;
  }
  videoRendererFactory.reset();
  UpdateVideoFrameMetaData();
  mRenderSetuped = true;
  AV_LOGD_ID(TAG, mID, "Video Render init OK, type %d",
             static_cast<int>(mClusterType));
  return OK;
}

RED_ERR CRedRenderVideoHal::UpdateVideoFrameMetaData() {
  if (mMetaDataSetuped) {
    return OK;
  }
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();

  switch (mClusterType) {
  case RedRender::VRClusterTypeOpenGL:
  case RedRender::VRClusterTypeMetal:
  case RedRender::VRClusterTypeAVSBDL: {
    TrackInfo track_info = mMetaData->track_info[mMetaData->video_index];
    mVideoFrameMetaData.frameWidth = track_info.width;
    mVideoFrameMetaData.frameHeight = track_info.height;
    mVideoFrameMetaData.layerWidth = track_info.width;
    mVideoFrameMetaData.layerHeight = track_info.height;
    mVideoFrameMetaData.sar.den = track_info.sar_den;
    mVideoFrameMetaData.sar.num = track_info.sar_num;
    mVideoFrameMetaData.aspectRatio = RedRender::AspectRatioFill;
    mVideoFrameMetaData.rotationMode = RedRender::NoRotation;
    mVideoFrameMetaData.color_primaries = RedRender::VR_AVCOL_PRI_RESERVED0;
    mVideoFrameMetaData.color_trc = RedRender::VR_AVCOL_TRC_RESERVED0;
    switch (track_info.color_space) {
    case AVCOL_SPC_BT2020_NCL:
      mVideoFrameMetaData.color_space = RedRender::VR_AVCOL_SPC_BT2020_NCL;
      break;
    case AVCOL_SPC_BT2020_CL:
      mVideoFrameMetaData.color_space = RedRender::VR_AVCOL_SPC_BT2020_CL;
      break;
    case AVCOL_SPC_BT709:
      mVideoFrameMetaData.color_space = RedRender::VR_AVCOL_SPC_BT709;
      break;
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE240M:
      mVideoFrameMetaData.color_space = RedRender::VR_AVCOL_SPC_BT709;
      break;
    default:
      mVideoFrameMetaData.color_space = RedRender::VR_AVCOL_SPC_BT709;
      break;
    }
    if (track_info.color_range == AVCOL_RANGE_JPEG) {
      mVideoFrameMetaData.color_range = RedRender::VR_AVCOL_RANGE_JPEG;
    } else {
      mVideoFrameMetaData.color_range = RedRender::VR_AVCOL_RANGE_MPEG;
    }
    break;
  }
  case RedRender::VRClusterTypeMediaCodec:
    break;
  case RedRender::VRClusterTypeHarmonyVideoDecoder:
    break;
  default:
    AV_LOGE_ID(TAG, mID, "[%s:%d] VideoRendererTypeUnknown.\n", __FUNCTION__,
               __LINE__);
    break;
  }

  switch (mVideoState->stat.vdec_type) {
  case RED_PROPV_DECODER_AVCODEC:
    mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420p;
    break;
  case RED_PROPV_DECODER_MEDIACODEC:
    mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatMediaCodec;
    break;
  case RED_PROPV_DECODER_VIDEOTOOLBOX:
    mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420sp_vtb;
    break;
  case RED_PROPV_DECODER_HARMONY_VIDEO_DECODER:
    mVideoFrameMetaData.pixel_format =
        RedRender::VRPixelFormatHarmonyVideoDecoder;
    break;
  default:
    mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatUnknown;
    AV_LOGE_ID(TAG, mID, "[%s, %d]unkonwn vdec type .\n", __FUNCTION__,
               __LINE__);
    return NO_INIT;
  }

  if (player_config->video_hdr_enable &&
      mVideoState->stat.pixel_format == AV_PIX_FMT_YUV420P10LE) {
    mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420p10le;
  }

  mRenderSetuped = true;
  mMetaDataSetuped = true;
  AV_LOGI_ID(TAG, mID, "Video frame meta OK, type %d",
             static_cast<int>(mClusterType));
  return OK;
}

RED_ERR CRedRenderVideoHal::PerformStart() {
  std::unique_lock<std::mutex> lck(mPauseLock);
  mPaused = false;
  mVideoState->video_clock->SetClock(mVideoState->video_clock->GetClock());
  mVideoState->video_clock->SetPause(false);
  mPausedCond.notify_one();
  return OK;
}

RED_ERR CRedRenderVideoHal::PerformPause() {
  std::unique_lock<std::mutex> lck(mLock);
  mPaused = true;
  mVideoState->video_clock->SetPause(true);
  return OK;
}

RED_ERR CRedRenderVideoHal::PerformStop() {
  std::unique_lock<std::mutex> lck(mLock);
  mAbort = true;
  mCond.notify_all();
  mPausedCond.notify_all();
  mSurfaceSet.notify_all();
  return OK;
}

RED_ERR CRedRenderVideoHal::PerformFlush() {
  std::unique_lock<std::mutex> lck(mLock);
  mCond.notify_one();
  return OK;
}

RED_ERR
CRedRenderVideoHal::RenderFrame(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (ConvertPixelFormat(buffer) != OK) {
    return ME_ERROR;
  }

  switch (mClusterType) {
#if defined(__ANDROID__)
  case RedRender::VRClusterTypeMediaCodec: {
    mVideoMediaCodecBufferContext.buffer_index =
        ((CGlobalBuffer::MediaCodecBufferContext *)(buffer->opaque))
            ->buffer_index;
    mVideoMediaCodecBufferContext.media_codec =
        ((CGlobalBuffer::MediaCodecBufferContext *)(buffer->opaque))
            ->media_codec;
    mVideoMediaCodecBufferContext.decoder_serial =
        ((CGlobalBuffer::MediaCodecBufferContext *)(buffer->opaque))
            ->decoder_serial;
    mVideoMediaCodecBufferContext.decoder =
        ((CGlobalBuffer::MediaCodecBufferContext *)(buffer->opaque))->decoder;
    mVideoMediaCodecBufferContext.opaque =
        ((CGlobalBuffer::MediaCodecBufferContext *)(buffer->opaque))->opaque;
    mVideoMediaCodecBufferContext.release_output_buffer =
        [](RedRender::VideoMediaCodecBufferContext *context,
           bool render) -> void {
      reddecoder::MediaCodecBufferContext media_codec_ctx;
      media_codec_ctx.buffer_index = context->buffer_index;
      media_codec_ctx.decoder = context->decoder;
      media_codec_ctx.media_codec = context->media_codec;
      media_codec_ctx.decoder_serial = context->decoder_serial;

      void (*release_output_buffer)(
          reddecoder::MediaCodecBufferContext *context, bool render) =
          (void (*)(reddecoder::MediaCodecBufferContext *context,
                    bool render))context->opaque;
      release_output_buffer(&media_codec_ctx, render);
    };
    RedRender::VRError VRRet =
        mVideoRender->onRender(&mVideoMediaCodecBufferContext, true);
    if (VRRet != RedRender::VRErrorNone) {
      notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY,
                     static_cast<int32_t>(VRRet));
      AV_LOGE_ID(TAG, mID, "setBufferProxy error\n");
    }
    if (buffer->opaque) {
      delete (CGlobalBuffer::MediaCodecBufferContext *)buffer->opaque;
      buffer->opaque = nullptr;
    }
    break;
  }
#endif
#if defined(__HARMONY__)
  case RedRender::VRClusterTypeHarmonyVideoDecoder: {
    mVideoHarmonyDecoderBufferContext.buffer_index =
        ((CGlobalBuffer::HarmonyMediaBufferContext *)(buffer->opaque))
            ->buffer_index;
    mVideoHarmonyDecoderBufferContext.video_decoder =
        ((CGlobalBuffer::HarmonyMediaBufferContext *)(buffer->opaque))
            ->video_decoder;
    mVideoHarmonyDecoderBufferContext.decoder_serial =
        ((CGlobalBuffer::HarmonyMediaBufferContext *)(buffer->opaque))
            ->decoder_serial;
    mVideoHarmonyDecoderBufferContext.decoder =
        ((CGlobalBuffer::HarmonyMediaBufferContext *)(buffer->opaque))->decoder;
    mVideoHarmonyDecoderBufferContext.opaque =
        ((CGlobalBuffer::HarmonyMediaBufferContext *)(buffer->opaque))->opaque;
    mVideoHarmonyDecoderBufferContext.release_output_buffer =
        [](RedRender::VideoHarmonyDecoderBufferContext *context,
           bool render) -> void {
      reddecoder::HarmonyVideoDecoderBufferContext video_decoder_ctx;
      video_decoder_ctx.buffer_index = context->buffer_index;
      video_decoder_ctx.decoder = context->decoder;
      video_decoder_ctx.video_decoder = context->video_decoder;
      video_decoder_ctx.decoder_serial = context->decoder_serial;
      void (*release_output_buffer)(
          reddecoder::HarmonyVideoDecoderBufferContext *context, bool render) =
          (void (*)(reddecoder::HarmonyVideoDecoderBufferContext *context,
                    bool render))context->opaque;
      release_output_buffer(&video_decoder_ctx, render);
    };

    RedRender::VRError VRRet =
        mVideoRender->onRender(&mVideoHarmonyDecoderBufferContext, true);
    if (VRRet != RedRender::VRErrorNone) {
      notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY, (int32_t)VRRet);
      AV_LOGE_ID(TAG, mID, "[RedPlayer_KLog][%s][%d] releaseBufferProxy error ",
                 __FUNCTION__, __LINE__);
    }
    if (buffer->opaque) {
      delete (CGlobalBuffer::HarmonyMediaBufferContext *)buffer->opaque;
      buffer->opaque = nullptr;
    }
  } break;
#endif
  case RedRender::VRClusterTypeOpenGL:
  case RedRender::VRClusterTypeMetal:
  case RedRender::VRClusterTypeAVSBDL: {
    if (buffer->pixel_format == CGlobalBuffer::kYUV420P10LE) {
      mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420p10le;
    } else if (buffer->pixel_format == CGlobalBuffer::kVTBBuffer) {
      mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420sp_vtb;
#if defined(__APPLE__)
      mVideoFrameMetaData.pixel_buffer =
          (CVPixelBufferRef)((CGlobalBuffer::VideoToolBufferContext
                                  *)(buffer->opaque))
              ->buffer;
#endif
    } else if (buffer->pixel_format == CGlobalBuffer::kYUV420 ||
               buffer->pixel_format == CGlobalBuffer::kYUVJ420P) {
      mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420p;
    }
#if defined(__APPLE__)
    mVideoFrameMetaData.pixel_buffer =
        (CVPixelBufferRef)((CGlobalBuffer::VideoToolBufferContext
                                *)(buffer->opaque))
            ->buffer;
#endif
    mVideoFrameMetaData.pitches[0] = buffer->yBuffer;
    mVideoFrameMetaData.pitches[1] = buffer->uBuffer;
    mVideoFrameMetaData.pitches[2] = buffer->vBuffer;
    mVideoFrameMetaData.linesize[0] = buffer->yStride;
    mVideoFrameMetaData.linesize[1] = buffer->uStride;
    mVideoFrameMetaData.linesize[2] = buffer->vStride;
    mVideoFrameMetaData.frameWidth = buffer->width;
    mVideoFrameMetaData.frameHeight = buffer->height;
    mVideoFrameMetaData.layerWidth = buffer->width;
    mVideoFrameMetaData.layerHeight = buffer->height;

    RedRender::VRError VRRet = mVideoRender->onInputFrame(&mVideoFrameMetaData);
    if (VRRet == RedRender::VRErrorNone) {
      VRRet = mVideoRender->onRender();
      if (VRRet != RedRender::VRErrorNone) {
        notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY,
                       static_cast<int32_t>(VRRet));
        AV_LOGE_ID(TAG, mID, "onRender error\n");
      }
    } else {
      notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY,
                     static_cast<int32_t>(VRRet));
      AV_LOGE_ID(TAG, mID, "onInputFrame error\n");
    }
#if defined(__APPLE__)
    if (buffer->pixel_format == CGlobalBuffer::kVTBBuffer && buffer->opaque &&
        ((CGlobalBuffer::VideoToolBufferContext *)(buffer->opaque))->buffer) {
      CVBufferRelease((CVPixelBufferRef)((CGlobalBuffer::VideoToolBufferContext
                                              *)(buffer->opaque))
                          ->buffer);
      delete (CGlobalBuffer::VideoToolBufferContext *)buffer->opaque;
      buffer->opaque = nullptr;
    }
#endif
    break;
  }
  case RedRender::VRClusterTypeUnknown:
  default:
    AV_LOGE_ID(TAG, mID, "[%s:%d] VideoRendererTypeUnknown.\n", __FUNCTION__,
               __LINE__);
    break;
  }
  if (mVideoState->latest_video_seek_load_serial == buffer->serial) {
    int latest_video_seek_load_serial =
        mVideoState->latest_video_seek_load_serial.exchange(
            -1, std::memory_order_seq_cst);
    if (latest_video_seek_load_serial == buffer->serial) {
      mVideoState->stat.latest_seek_load_duration =
          (CurrentTimeUs() - mVideoState->latest_seek_load_start_at) / 1000;
      AV_LOGI_ID(TAG, mID,
                 "video seek complete, cost %" PRId64 "ms, serial %d\n",
                 mVideoState->stat.latest_seek_load_duration,
                 latest_video_seek_load_serial);
      if (getMasterSyncType(mVideoState) == CLOCK_VIDEO) {
        notifyListener(RED_MSG_VIDEO_SEEK_RENDERING_START, 1);
      } else {
        notifyListener(RED_MSG_VIDEO_SEEK_RENDERING_START, 0);
      }
    }
  }
  mVideoState->stat.vfps = mSpeedSampler.add();
  if (!mVideoState->first_video_frame_rendered) {
    mVideoState->first_video_frame_rendered = 1;
    notifyListener(RED_MSG_VIDEO_RENDERING_START);
    notifyListener(RED_MSG_VIDEO_START_ON_PLAYING);
  }
  if (mPaused) {
    while (mPaused && !mAbort && !mVideoState->step_to_next_frame) {
      std::unique_lock<std::mutex> lck(mPauseLock);
      mPausedCond.wait_for(lck, std::chrono::milliseconds(20));
    }
    if (mAbort) {
      return OK;
    }
    if (mVideoState->step_to_next_frame) {
      mVideoState->step_to_next_frame = false;
    }
  }
  return OK;
}

RED_ERR
CRedRenderVideoHal::ConvertPixelFormat(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (buffer->pixel_format == CGlobalBuffer::kYUV420P10LE &&
      mClusterType != RedRender::VRClusterTypeAVSBDL) {
    mVideoFrameMetaData.pixel_format = RedRender::VRPixelFormatYUV420p;
    buffer->pixel_format = CGlobalBuffer::kYUV420;

    AVFrame *src_frame = reinterpret_cast<AVFrame *>(
        (reinterpret_cast<CGlobalBuffer::FFmpegBufferContext *>(buffer->opaque))
            ->av_frame);
    AVFrame *dst_frame = av_frame_alloc();
    dst_frame->format = AV_PIX_FMT_YUV420P;
    dst_frame->width = buffer->width;
    dst_frame->height = buffer->height;
    av_frame_get_buffer(dst_frame, 32);

    if (!mSwsContext) {
      AV_LOGI_ID(TAG, mID, "Convert yuv420p10le to yuv420p.\n");
    }
    mSwsContext = sws_getCachedContext(
        mSwsContext, src_frame->width, src_frame->height,
        (AVPixelFormat)src_frame->format, dst_frame->width, dst_frame->height,
        (AVPixelFormat)dst_frame->format, SWS_BICUBIC, NULL, NULL, NULL);

    if (!mSwsContext ||
        !sws_scale(mSwsContext, src_frame->data, src_frame->linesize, 0,
                   src_frame->height, dst_frame->data, dst_frame->linesize)) {
      AV_LOGE_ID(TAG, mID, "Failed to convert pixelformat.\n");
      av_frame_unref(dst_frame);
      av_frame_free(&dst_frame);
      return ME_ERROR;
    }

    buffer->width = dst_frame->width;
    buffer->height = dst_frame->height;
    buffer->yStride = dst_frame->linesize[0];
    buffer->uStride = dst_frame->linesize[1];
    buffer->vStride = dst_frame->linesize[2];
    buffer->yBuffer = dst_frame->data[0];
    buffer->uBuffer = dst_frame->data[1];
    buffer->vBuffer = dst_frame->data[2];
    (reinterpret_cast<CGlobalBuffer::FFmpegBufferContext *>(buffer->opaque))
        ->av_frame = reinterpret_cast<void *>(dst_frame);

    av_frame_unref(src_frame);
    av_frame_free(&src_frame);
  }
  return OK;
}

void CRedRenderVideoHal::notifyListener(uint32_t what, int32_t arg1,
                                        int32_t arg2, void *obj1, void *obj2,
                                        int obj1_len, int obj2_len) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  if (mNotifyCb) {
    mNotifyCb(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

void CRedRenderVideoHal::setConfig(const sp<CoreGeneralConfig> &config) {
  std::lock_guard<std::mutex> lck(mLock);
  mGeneralConfig = config;
}

void CRedRenderVideoHal::setNotifyCb(NotifyCallback notify_cb) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  mNotifyCb = notify_cb;
}

void CRedRenderVideoHal::release() {
  AV_LOGD_ID(TAG, mID, "%s start\n", __func__);
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      AV_LOGD_ID(TAG, mID, "%s already released, just return.\n", __func__);
      return;
    }
    mReleased = true;
  }
  if (mThread.joinable()) {
    mThread.join();
  }
}

// base method
void CRedRenderVideoHal::ThreadFunc() {
  if (Init() != OK || !mVideoRender) {
    notifyListener(RED_MSG_ERROR, ERROR_VIDEO_DISPLAY);
    mMetaData.reset();
    AV_LOGE_ID(TAG, mID, "[%s:%d]  Init error .\n", __FUNCTION__, __LINE__);
    return;
  }
  UpdateVideoFrameMetaData();

#if defined(__APPLE__)
  if (mVideoRender) {
    if (mClusterType == RedRender::VRClusterTypeOpenGL) {
      mVideoRender->attachFilter(RedRender::VideoOpenGLDeviceFilterType,
                                 &mVideoFrameMetaData, 0);
    } else if (mClusterType == RedRender::VRClusterTypeMetal) {
      mVideoRender->attachFilter(RedRender::VideoMetalDeviceFilterType,
                                 &mVideoFrameMetaData, 0);
    }
  }
#endif

  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  int64_t framedrop = player_config ? player_config->framedrop : 0;
  int64_t prev_check_unsync_time = 0;
  double delay = 0.0;
  double duration = 0.0;
  double remaining_time = 0.0;

#if defined(__APPLE__)
  pthread_setname_np("videorender");
#elif defined(__ANDROID__) || defined(__HARMONY__)
  pthread_setname_np(pthread_self(), "videorender");
#endif

  while (!mAbort) {
#if defined(__ANDROID__) || defined(__HARMONY__)
    if (mVideoRender && mClusterType == RedRender::VRClusterTypeOpenGL) {
      std::unique_lock<std::mutex> lck(mSurfaceLock);
      if (mSurfaceUpdate) {
        auto window = mNativeWindow ? mNativeWindow->get() : nullptr;
        RedRender::VRError ret = mVideoRender->setSurface(window);
        mSurfaceUpdate = false;
        if (ret != RedRender::VRErrorNone) {
          if (!mAbort) {
            AV_LOGD_ID(TAG, mID,
                       "setSurface %p failed, wait for surface update\n",
                       window);
            mSurfaceSet.wait(lck);
            continue;
          }
        }
      }
    }
#endif
    RED_ERR ret = OK;
    std::unique_ptr<CGlobalBuffer> in_buffer_;
    ret = ReadFrame(in_buffer_);
    if (ret == ME_CLOSED) {
      std::unique_lock<std::mutex> lck(mLock);
      if (!mAbort) {
        mCond.wait(lck);
      }
      continue;
    } else if (ret != OK || !in_buffer_) {
      usleep(20 * 1000);
      continue;
    }
    if (in_buffer_->serial != mVideoProcesser->getSerial()) {
      continue;
    }
    while (!mAbort) {
#if defined(__ANDROID__) || defined(__HARMONY__)
      {
        std::unique_lock<std::mutex> lck(mSurfaceLock);
        if (mSurfaceUpdate && mVideoRender && mNativeWindow &&
            mClusterType == RedRender::VRClusterTypeOpenGL) {
          mVideoRender->setSurface(mNativeWindow->get());
          mSurfaceUpdate = false;
        }
      }
#endif
      if (in_buffer_->serial != mVideoProcesser->getSerial()) {
        break;
      }

      double time = 0.0;

      duration = ComputeDuration(in_buffer_);
      delay = ComputeDelay(duration);
      time = CurrentTimeUs() / 1000000.0;

      if (!isnan(mVideoState->stat.avdiff)) {
        if (std::abs(mVideoState->stat.avdiff) > 1.0 &&
            CurrentTimeMs() - prev_check_unsync_time > 1000) {
          AV_LOGI_ID(TAG, mID, "av unsync A: %f, V: %f\n",
                     getMasterClock(mVideoState),
                     mVideoState->video_clock->GetClock());
          prev_check_unsync_time = CurrentTimeMs();
        }
      }

      if (in_buffer_->serial != mSerial) {
        mFrameTick.time = CurrentTimeUs() / 1000000.0;
        mSerial = in_buffer_->serial;
      }

      if (mFrameTick.time <= FLT_EPSILON || time < mFrameTick.time) {
        mFrameTick.time = time;
      }

      if (time < mFrameTick.time + delay && !mForceRefresh) {
        remaining_time = std::min(mFrameTick.time + delay - time, REFRESH_RATE);
        if (remaining_time * 1000000 > 0) {
          usleep(remaining_time * 1000000);
        }
        continue;
      }

      mFrameTick.time += delay;

      if (delay > 0 && time - mFrameTick.time > AV_SYNC_THRESHOLD_MAX) {
        mFrameTick.time = time;
      }

      mFrameTick.pts = in_buffer_->pts / 1000.0;
      mFrameTick.serial = in_buffer_->serial;
      mFrameTick.duration = duration;

      mVideoState->video_clock->SetClock(in_buffer_->pts / 1000.0);
      mVideoState->video_clock->SetClockSerial(in_buffer_->serial);
      if (!mVideoState->video_clock->GetClockAvaliable()) {
        mVideoState->video_clock->SetClockAvaliable(true);
      }

      if (mVideoProcesser->frameQueue() &&
          mVideoProcesser->frameQueue()->size() > 0) {
        if ((framedrop > 0 ||
             (framedrop && getMasterSyncType(mVideoState) != CLOCK_VIDEO)) &&
            time > mFrameTick.time + duration) {
          AV_LOGI_ID(TAG, mID,
                     "video late drop frame pts %f, delay %f, duration %f, "
                     "time %f, mFrameTick.time %f\n",
                     in_buffer_->pts / 1000.0, delay, duration, time,
                     mFrameTick.time);
          break;
        }
      }

      RenderFrame(in_buffer_);
      mForceRefresh = false;
      break;
    }
  }
  if (mVideoRender && (mClusterType == RedRender::VRClusterTypeOpenGL ||
                       mClusterType == RedRender::VRClusterTypeMetal)) {
    mVideoRender->close();
    mVideoRender->detachAllFilter();
    mVideoRender->releaseContext();
  }
  AV_LOGD_ID(TAG, mID, "Video Render thread Exit.");
}

RED_ERR CRedRenderVideoHal::pause() { return PerformPause(); }
RED_ERR CRedRenderVideoHal::start() { return PerformStart(); }
RED_ERR CRedRenderVideoHal::stop() { return PerformStop(); }
RED_ERR CRedRenderVideoHal::flush() { return PerformFlush(); }

REDPLAYER_NS_END;
