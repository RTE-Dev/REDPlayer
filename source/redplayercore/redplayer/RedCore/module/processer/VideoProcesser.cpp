/*
 * VideoProcesser.cpp
 *
 *  Created on: 2022年8月19日
 *      Author: liuhongda
 */

#include "VideoProcesser.h"
#include "RedMsg.h"
#include "RedProp.h"
#include "base/RedConfig.h"

#define TAG "VideoProcesser"
#define MAX_PKT_QUEUE_DEEP (350)
#define CODEC_CONFIGURE_TIMEOUT_TIME_MS (3000)
#define CODEC_RETRY_TIMES (20)

REDPLAYER_NS_BEGIN;

CVideoProcesser::CVideoProcesser(int id,
                                 sp<CRedSourceController> &sourceManager,
                                 const sp<VideoState> &state,
                                 NotifyCallback notify_cb, int serial)
    : mID(id), mRedSourceController(sourceManager), mVideoState(state),
      mNotifyCb(notify_cb) {
  mSerial = serial;
}

CVideoProcesser::~CVideoProcesser() {
  mFrameQueue.reset();
  mVideoDecoder.reset();
  mRedSourceController.reset();
  mPendingPkt.reset();
  mBuffer.reset();
  mMetaData.reset();
#if defined(__ANDROID__)
  mCurNativeWindow.reset();
  mPrevNativeWindow.reset();
#endif
}

RED_ERR CVideoProcesser::Prepare(sp<MetaData> &metadata) {
  RED_ERR ret = OK;
  size_t queue_size = VIDEO_PICTURE_QUEUE_SIZE_DEFAULT;
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      return OK;
    }
  }
  mMetaData = metadata;
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (mMetaData && mMetaData->video_index >= 0) {
    auto track_info = mMetaData->track_info[mMetaData->video_index];
    mHeight = track_info.height;
    mWidth = track_info.width;
  }
  mFrameQueue = std::make_shared<FrameQueue>(queue_size, TYPE_VIDEO);
  if (!mFrameQueue) {
    AV_LOGE_ID(TAG, mID, "Video frame queue create error\n");
    return ME_ERROR;
  }
  ret = Init();
  if (ret != OK) {
    player_config->videotoolbox = !player_config->videotoolbox;
    player_config->enable_ndkvdec = !player_config->enable_ndkvdec;
    ret = Init();
    if (ret != OK) {
      notifyListener(RED_MSG_ERROR, ERROR_STREAM_OPEN);
      return ret;
    }
  }
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      return OK;
    }
    this->run();
  }
  return ret;
}

RED_ERR CVideoProcesser::Init() {
  if (!mMetaData) {
    AV_LOGE_ID(TAG, mID, "[%s][%d] null metadata\n", __FUNCTION__, __LINE__);
    return ME_ERROR;
  }
  if (mReleased) {
    return OK;
  }

  std::string codec_module, codec_mime;
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  int codec_id = mMetaData->track_info[mMetaData->video_index].codec_id;
  reddecoder::VideoCodecName codec_name = reddecoder::VideoCodecName::kUnknown;
  reddecoder::VideoCodecImplementationType type =
      reddecoder::VideoCodecImplementationType::kUnknown;

  if (codec_id == AV_CODEC_ID_H264) {
    codec_name = reddecoder::VideoCodecName::kH264;
  } else if (codec_id == AV_CODEC_ID_HEVC) {
    codec_name = reddecoder::VideoCodecName::kH265;
    mIsHevc = true;
  } else {
    codec_name = reddecoder::VideoCodecName::kFFMPEGID;
  }

  if (codec_name == reddecoder::VideoCodecName::kUnknown) {
    AV_LOGE_ID(TAG, mID, "Unsupport codec id %d!\n", codec_id);
    return ME_ERROR;
  }

  codec_mime = avcodec_get_name((AVCodecID)codec_id);

  if (player_config) {
    if (player_config->videotoolbox) {
      type = reddecoder::VideoCodecImplementationType::kHardwareiOS;
      mVideoState->stat.vdec_type = RED_PROPV_DECODER_VIDEOTOOLBOX;
      codec_module = VTB_MODULE_NAME;
    }
#if defined(__ANDROID__)
    else if (player_config->enable_ndkvdec) {
      type = reddecoder::VideoCodecImplementationType::kHardwareAndroid;
      mVideoState->stat.vdec_type = RED_PROPV_DECODER_MEDIACODEC;
      codec_module = MEDIACODEC_MODULE_NAME;
      if (!mCurNativeWindow || !mCurNativeWindow->get()) {
        AV_LOGD_ID(TAG, mID, "init decoder with null surface\n");
      }
    }
#endif
    else {
      type = reddecoder::VideoCodecImplementationType::kSoftware;
      mVideoState->stat.vdec_type = RED_PROPV_DECODER_AVCODEC;
      codec_module = AVCODEC_MODULE_NAME;
    }
  }

  std::unique_ptr<reddecoder::VideoDecoderFactory> decoder_factory =
      std::make_unique<reddecoder::VideoDecoderFactory>();
  reddecoder::VideoCodecInfo codec_info(codec_name, type);
  codec_info.ffmpeg_codec_id = codec_id;
  mVideoDecoder = decoder_factory->create_video_decoder(codec_info);
  if (!mVideoDecoder) {
    AV_LOGE_ID(TAG, mID, "Video decoder create error\n");
    notifyListener(RED_MSG_ERROR, ERROR_DECODER_VDEC,
                   -(static_cast<int32_t>(DECODER_INIT_ERROR)));
    return ME_ERROR;
  }

  reddecoder::VideoCodecError err = mVideoDecoder->init();
  if (err != reddecoder::VideoCodecError::kNoError) {
    AV_LOGE_ID(TAG, mID, "Video decoder init error %d\n", (int)err);
    return ME_ERROR;
  }

  err = mVideoDecoder->register_decode_complete_callback(this);
  if (err != reddecoder::VideoCodecError::kNoError) {
    AV_LOGE_ID(TAG, mID, "Video decoder init error %d\n", (int)err);
    return ME_ERROR;
  }

  if (mVideoState->stat.vdec_type != RED_PROPV_DECODER_MEDIACODEC) {
    ResetDecoderFormat();
  }

  if (player_config->video_hdr_enable) {
    if (mMetaData->track_info[mMetaData->video_index].pixel_format ==
        AV_PIX_FMT_YUV420P10LE) {
      mVideoState->stat.pixel_format = AV_PIX_FMT_YUV420P10LE;
    }
  }

  mVideoState->video_codec_info = codec_module + ", " + codec_mime;
  AV_LOGI_ID(TAG, mID, "Video decoder create success, codec name %s\n",
             codec_module.c_str());

#if defined(__APPLE__)
  notifyListener(
      RED_MSG_VIDEO_DECODER_OPEN,
      static_cast<int32_t>(
          type == reddecoder::VideoCodecImplementationType::kHardwareiOS));
#endif

  return OK;
}

RED_ERR CVideoProcesser::DecoderTransmit(AVPacket *pkt) {
  RED_ERR ret = OK;
  if (!mVideoDecoder)
    return ME_ERROR;

  if (!mPendingPkt || !mBuffer) {
    mBuffer.reset();
    mBuffer = std::make_unique<reddecoder::Buffer>(
        reddecoder::BufferType::kVideoPacket, pkt->data, pkt->size, false);
    mBuffer->get_video_packet_meta()->pts_ms = pkt->pts;
    mBuffer->get_video_packet_meta()->dts_ms = pkt->dts;
    mBuffer->get_video_packet_meta()->format =
        reddecoder::VideoPacketFormat::kFollowExtradataFormat;
  } else {
    mBuffer->get_video_packet_meta()->format =
        reddecoder::VideoPacketFormat::kAnnexb;
  }

  if (mRefreshSession) {
    mBuffer->get_video_packet_meta()->decode_flags |=
        static_cast<uint32_t>(reddecoder::DecodeFlag::kDoNotOutputFrame);
  }
  auto err = mVideoDecoder->decode(mBuffer.get());

#if defined(__ANDROID__)
  if (mSurfaceUpdated) {
    return ret;
  }
#endif

  switch (err) {
  case reddecoder::VideoCodecError::kNoError:
  case reddecoder::VideoCodecError::kEOF:
    break;
  case reddecoder::VideoCodecError::kTryAgain:
    ret = ME_RETRY;
    break;
  case reddecoder::VideoCodecError::kNeedIFrame:
    mRefreshSession = true;
    break;
  default:
    mDecoderErrorCount++;
    if (mVideoState->stat.vdec_type != RED_PROPV_DECODER_AVCODEC) {
      notifyListener(RED_MSG_ERROR, ERROR_DECODER_VDEC,
                     -(static_cast<int32_t>(err)));
    }
    break;
  }
  return ret;
}

reddecoder::VideoCodecError CVideoProcesser::on_decoded_frame(
    std::unique_ptr<reddecoder::Buffer> decoded_frame) {

  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  mVideoState->stat.vdps = mSpeedSampler.add();
  mDecoderErrorCount = 0;
  auto meta = decoded_frame->get_video_frame_meta();
  std::unique_ptr<CGlobalBuffer> buffer(new CGlobalBuffer());
  if (!buffer) {
    return reddecoder::VideoCodecError::kAllocateBufferError;
  }

  buffer->width = meta->width;
  buffer->height = meta->height;
  buffer->data = decoded_frame->get_away_data();
  buffer->datasize = decoded_frame->get_size();

  auto track_info = mMetaData->track_info[mMetaData->video_index];
  AVRational tb =
      (AVRational){track_info.time_base_num, track_info.time_base_den};
  buffer->pts = meta->pts_ms * av_q2d(tb) * 1000;
  buffer->serial = mSerial;

  buffer->uBuffer = meta->uBuffer;
  buffer->vBuffer = meta->vBuffer;
  buffer->yBuffer = meta->yBuffer;
  buffer->yStride = meta->yStride;
  buffer->uStride = meta->uStride;
  buffer->vStride = meta->vStride;
  buffer->pixel_format = CGlobalBuffer::kUnknow;

  switch (meta->pixel_format) {
  case reddecoder::PixelFormat::kVTBBuffer:
    buffer->pixel_format = CGlobalBuffer::kVTBBuffer;
    if (meta->buffer_context) {
      auto buffer_context = new CGlobalBuffer::VideoToolBufferContext();
      buffer_context->buffer =
          ((reddecoder::VideoToolBufferContext *)meta->buffer_context)->buffer;
      buffer->opaque = buffer_context;
      delete (reddecoder::VideoToolBufferContext *)meta->buffer_context;
    }
    break;
  case reddecoder::PixelFormat::kMediaCodecBuffer:
    buffer->pixel_format = CGlobalBuffer::kMediaCodecBuffer;
#if defined(__ANDROID__)
    if (meta->buffer_context) {
      auto buffer_context = new CGlobalBuffer::MediaCodecBufferContext();
      auto od_buffer_context =
          ((reddecoder::MediaCodecBufferContext *)meta->buffer_context);
      buffer_context->buffer_index = od_buffer_context->buffer_index;
      buffer_context->decoder = od_buffer_context->decoder;
      buffer_context->decoder_serial = od_buffer_context->decoder_serial;
      buffer_context->media_codec = od_buffer_context->media_codec;
      buffer_context->opaque =
          reinterpret_cast<void *>(od_buffer_context->release_output_buffer);

      buffer_context->release_output_buffer =
          [](CGlobalBuffer::MediaCodecBufferContext *ctx, bool render) -> void {
        reddecoder::MediaCodecBufferContext media_codec_ctx;
        media_codec_ctx.buffer_index = ctx->buffer_index;
        media_codec_ctx.decoder = ctx->decoder;
        media_codec_ctx.media_codec = ctx->media_codec;
        media_codec_ctx.decoder_serial = ctx->decoder_serial;
        void (*release_output_buffer)(
            reddecoder::MediaCodecBufferContext *context, bool render) =
            (void (*)(reddecoder::MediaCodecBufferContext *context,
                      bool render))ctx->opaque;
        release_output_buffer(&media_codec_ctx, render);
      };
      buffer->opaque = buffer_context;
      delete (reddecoder::MediaCodecBufferContext *)meta->buffer_context;
    }
#endif
    break;
  case reddecoder::PixelFormat::kYUV420:
    buffer->pixel_format = CGlobalBuffer::kYUV420;
  case reddecoder::PixelFormat::kYUVJ420P:
    if (buffer->pixel_format == CGlobalBuffer::kUnknow) {
      buffer->pixel_format = CGlobalBuffer::kYUVJ420P;
    }
  case reddecoder::PixelFormat::kYUV420P10LE:
    if (buffer->pixel_format == CGlobalBuffer::kUnknow) {
      buffer->pixel_format = CGlobalBuffer::kYUV420P10LE;
    }
    if (meta->buffer_context) {
      auto buffer_context = new CGlobalBuffer::FFmpegBufferContext();
      auto od_buffer_context =
          ((reddecoder::FFmpegBufferContext *)meta->buffer_context);
      buffer_context->av_frame = od_buffer_context->av_frame;
      buffer_context->opaque =
          reinterpret_cast<void *>(od_buffer_context->release_av_frame);

      buffer_context->release_av_frame =
          [](CGlobalBuffer::FFmpegBufferContext *ctx) -> void {
        reddecoder::FFmpegBufferContext ffmpeg_ctx;
        ffmpeg_ctx.av_frame = ctx->av_frame;
        void (*release_av_frame)(reddecoder::FFmpegBufferContext *context) =
            (void (*)(reddecoder::FFmpegBufferContext *context))ctx->opaque;
        release_av_frame(&ffmpeg_ctx);
      };
      buffer->opaque = buffer_context;
      delete (reddecoder::FFmpegBufferContext *)meta->buffer_context;
    }
    break;
  default:
    AV_LOGW_ID(TAG, mID, "unsupportted frame format, format=%d\n",
               meta->pixel_format);
    break;
  }

  if (mAbort) {
    return reddecoder::VideoCodecError::kNoError;
  }

  if (!mFrameQueue) {
    return reddecoder::VideoCodecError::kInitError;
  }

  if ((player_config->framedrop > 0) ||
      (player_config->framedrop &&
       getMasterSyncType(mVideoState) != CLOCK_VIDEO)) {
    mVideoState->stat.decode_frame_count++;
    double dpts = buffer->pts / 1000;
    double diff = dpts - getMasterClock(mVideoState);
    if (!isnan(diff) && std::abs(diff) < AV_NOSYNC_THRESHOLD &&
        mVideoState->stat.video_cache.packets > 0 &&
        mSerial == getMasterClockSerial(mVideoState) &&
        mVideoState->first_video_frame_rendered &&
        getMasterClockAvaliable(mVideoState)) {
      mVideoState->frame_drops_early++;
      mVideoState->continuous_frame_drops_early++;
      if (mVideoState->continuous_frame_drops_early >
          player_config->framedrop) {
        mVideoState->continuous_frame_drops_early = 0;
      } else {
        mVideoState->stat.drop_frame_count++;
        mVideoState->stat.drop_frame_rate =
            static_cast<float>(mVideoState->stat.drop_frame_count) /
            static_cast<float>(mVideoState->stat.decode_frame_count);
        AV_LOGW_ID(TAG, mID, "drop frame early pts %lf, diff %lf\n", dpts,
                   diff);
        return reddecoder::VideoCodecError::kNoError;
      }
    }
  }

  if (checkAccurateSeek(buffer)) {
    return reddecoder::VideoCodecError::kNoError;
  }

  if (mDecoderRecovery) {
    return reddecoder::VideoCodecError::kNoError;
  }

  if (mWidth != buffer->width || mHeight != buffer->height) {
    mWidth = buffer->width;
    mHeight = buffer->height;
    notifyListener(RED_MSG_VIDEO_SIZE_CHANGED, meta->width, meta->height);
  }

  if (!mFirstFrameDecoded) {
    mFirstFrameDecoded = true;
    notifyListener(RED_MSG_VIDEO_DECODED_START);
  }
  mFrameQueue->putFrame(buffer);
  return reddecoder::VideoCodecError::kNoError;
}

void CVideoProcesser::on_decode_error(reddecoder::VideoCodecError error,
                                      int internal_error_code) {
  if (mAbort) {
    return;
  }
#if defined(__ANDROID__)
  if (mSurfaceUpdated) {
    return;
  }
#endif
  switch (error) {
  case reddecoder::VideoCodecError::kNoError:
  case reddecoder::VideoCodecError::kEOF:
  case reddecoder::VideoCodecError::kTryAgain:
    break;
  case reddecoder::VideoCodecError::kNeedIFrame:
    mRefreshSession = true;
    break;
  default:
    mDecoderErrorCount++;
    if (mVideoState->stat.vdec_type != RED_PROPV_DECODER_AVCODEC) {
      notifyListener(RED_MSG_ERROR, ERROR_DECODER_VDEC,
                     -(static_cast<int32_t>(error)));
    }
    break;
  }
}

RED_ERR
CVideoProcesser::ReadPacketOrBuffering(std::unique_ptr<RedAvPacket> &pkt) {
  if (!mRedSourceController)
    return ME_ERROR;
  RED_ERR ret = mRedSourceController->getPacket(pkt, TYPE_VIDEO, false);
  if (ret == ME_RETRY && !mEOF) {
    if (mVideoState->first_video_frame_rendered == 1) {
      mRedSourceController->toggleBuffering(true);
    }
    ret = mRedSourceController->getPacket(pkt, TYPE_VIDEO, true);
  }
  return ret;
}

RED_ERR CVideoProcesser::PerformStop() {
  std::unique_lock<std::mutex> lck(mLock);

  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (player_config->enable_accurate_seek) {
    std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
    mVideoState->video_accurate_seek_req = false;
    mVideoState->video_accurate_seek_cond.notify_all();
  }
  mAbort = true;
  mCond.notify_all();
  mSurfaceSet.notify_one();
  if (mFrameQueue) {
    mFrameQueue->abort();
  }
  return OK;
}
RED_ERR CVideoProcesser::PerformFlush() {
  std::lock_guard<std::mutex> lck(mLock);
  RED_ERR ret = OK;
  mSerial++;
  mEOF = false;
  mFinishedSerial = -1;
  if (mVideoDecoder &&
      (mInputPacketCount > 0 ||
       mVideoState->stat.vdec_type != RED_PROPV_DECODER_MEDIACODEC)) {
    mVideoDecoder->flush();
  }
  if (mFrameQueue) {
    mFrameQueue->flush();
  }
  mInputPacketCount = 0;
  if (mVideoState->pause_req) {
    mVideoState->step_to_next_frame = true;
  }
  AV_LOGI_ID(TAG, mID, "%s mSerial %d\n", __func__, mSerial);
  return ret;
}

RED_ERR CVideoProcesser::ResetDecoder() {
  if (mVideoState->stat.vdec_type == RED_PROPV_DECODER_MEDIACODEC &&
      mFrameQueue) {
    mFrameQueue->flush();
  }
  return ResetDecoderFormat();
}

RED_ERR CVideoProcesser::ResetDecoderFormat() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  RED_ERR ret = OK;
  if (!mVideoDecoder) {
    return ME_ERROR;
  }

  if (!mMetaData) {
    return ME_RETRY;
  }
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();

  auto track_info = mMetaData->track_info[mMetaData->video_index];
  reddecoder::Buffer buffer(reddecoder::BufferType::kVideoFormatDesc,
                            track_info.extra_data, track_info.extra_data_size,
                            false);
  auto buffer_meta = buffer.get_video_format_desc_meta();
  buffer_meta->height = track_info.height;
  buffer_meta->width = track_info.width;
  buffer_meta->rotate_degree = track_info.rotation;
  buffer_meta->items.push_back(
      {0, 0, reddecoder::VideoFormatDescType::kExtraData});

  switch (track_info.color_primaries) {
  case AVCOL_PRI_BT2020:
    buffer_meta->color_primaries = reddecoder::VideoColorPrimaries::kBT2020;
    break;
  case AVCOL_PRI_BT709:
    buffer_meta->color_primaries = reddecoder::VideoColorPrimaries::kBT709;
    break;
  case AVCOL_PRI_SMPTE170M:
    buffer_meta->color_primaries = reddecoder::VideoColorPrimaries::kSMPTE170M;
    break;
  case AVCOL_PRI_SMPTE240M:
    buffer_meta->color_primaries = reddecoder::VideoColorPrimaries::kSMPTE240M;
    break;
  case AVCOL_SPC_BT470BG:
    buffer_meta->color_primaries = reddecoder::VideoColorPrimaries::kBT470BG;
    break;
  case AVCOL_PRI_SMPTE432:
    buffer_meta->color_primaries = reddecoder::VideoColorPrimaries::kSMPTE432;
    break;
  default:
    break;
  }

  switch (track_info.color_trc) {
  case AVCOL_TRC_SMPTEST2084:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kSMPTEST2084;
    break;
  case AVCOL_TRC_ARIB_STD_B67:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kARIB_STD_B67;
    break;
  case AVCOL_TRC_BT709:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kBT709;
    break;
  case AVCOL_TRC_BT2020_10:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kBT2020_10;
    break;
  case AVCOL_TRC_BT2020_12:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kBT2020_12;
    break;
  case AVCOL_TRC_SMPTE170M:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kSMPTE170M;
    break;
  case AVCOL_TRC_SMPTE240M:
    buffer_meta->color_trc =
        reddecoder::VideoColorTransferCharacteristic::kSMPTE240M;
    break;
  default:
    break;
  }

  switch (track_info.color_range) {
  case AVCOL_RANGE_MPEG:
    buffer_meta->color_range = reddecoder::VideoColorRange::kMPEG;
    break;
  case AVCOL_RANGE_JPEG:
    buffer_meta->color_range = reddecoder::VideoColorRange::kJPEG;
    break;
  default:
    buffer_meta->color_range = reddecoder::VideoColorRange::kMPEG;
    break;
  }

  switch (track_info.color_space) {
  case AVCOL_SPC_BT2020_NCL:
    buffer_meta->colorspace = reddecoder::VideoColorSpace::kBT2020_NCL;
    break;
  case AVCOL_SPC_BT2020_CL:
    buffer_meta->colorspace = reddecoder::VideoColorSpace::kBT2020_CL;
    break;
  case AVCOL_SPC_BT709:
    buffer_meta->colorspace = reddecoder::VideoColorSpace::kBT709;
    break;
  case AVCOL_SPC_SMPTE170M:
    buffer_meta->colorspace = reddecoder::VideoColorSpace::kSMPTE170M;
    break;
  case AVCOL_SPC_BT470BG:
    buffer_meta->colorspace = reddecoder::VideoColorSpace::kBT470BG;
    break;
  case AVCOL_SPC_SMPTE240M:
    buffer_meta->colorspace = reddecoder::VideoColorSpace::kSMPTE240M;
    break;
  default:
    break;
  }

  if (player_config->video_hdr_enable) {
    buffer_meta->is_hdr = player_config->video_hdr_enable;
  }
  buffer_meta->sample_aspect_ratio.den = track_info.sar_den;
  buffer_meta->sample_aspect_ratio.num = track_info.sar_num;
  mVideoDecoder->set_video_format_description(&buffer);
  mCodecConfigured = true;
  return ret;
}

void CVideoProcesser::DecodeLastCacheGop() {
  mDecoderRecovery = true;
  if ((mVideoState->stat.vdec_type == RED_PROPV_DECODER_VIDEOTOOLBOX) &&
      (!mPktQueue.empty()) &&
      mPktQueue.front()->IsKeyOrIdrPacket(mIdrBasedIdentified, mIsHevc)) {
    AV_LOGD_ID(TAG, mID, "%s start\n", __func__);
    for (auto it = mPktQueue.begin(); it != mPktQueue.end(); it++) {
      if (mAbort || pktQueueFrontIsFlush()) {
        break;
      }
      auto cache_pkt = (*it);
      DecoderTransmit(cache_pkt->GetAVPacket());
    }
    AV_LOGD_ID(TAG, mID, "%s end\n", __func__);
  }
  mDecoderRecovery = false;
}

void CVideoProcesser::notifyListener(uint32_t what, int32_t arg1, int32_t arg2,
                                     void *obj1, void *obj2, int obj1_len,
                                     int obj2_len) {
  std::lock_guard<std::mutex> lck(mNotifyCbLock);
  if (mNotifyCb) {
    mNotifyCb(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

bool CVideoProcesser::checkAccurateSeek(
    const std::unique_ptr<CGlobalBuffer> &buffer) {
  bool video_accurate_seek_fail = false;
  int64_t video_seek_pos = 0;
  int64_t now = 0;
  int64_t deviation = 0;
  int64_t deviation2 = 0;
  int64_t deviation3 = 0;
  double pts = buffer->pts / 1000;
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (player_config->enable_accurate_seek &&
      mVideoState->video_accurate_seek_req && !mVideoState->seek_req) {
    if (!isnan(pts)) {
      video_seek_pos = mVideoState->seek_pos;
      mVideoState->accurate_seek_vframe_pts = pts * 1000 * 1000;
      deviation = std::abs(
          (static_cast<int64_t>(pts * 1000 * 1000) - mVideoState->seek_pos));
      if (pts * 1000 * 1000 < mVideoState->seek_pos) {
        now = CurrentTimeMs();
        if (mVideoState->drop_vframe_count == 0) {
          std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
          if (mVideoState->accurate_seek_start_time <= 0 &&
              (mMetaData->audio_index < 0 ||
               mVideoState->audio_accurate_seek_req)) {
            mVideoState->accurate_seek_start_time = now;
          }
        }
        mVideoState->drop_vframe_count++;

        if ((now - mVideoState->accurate_seek_start_time) <=
            player_config->accurate_seek_timeout) {
          return true;
        } else {
          video_accurate_seek_fail = true;
        }
      } else {
        while (mVideoState->audio_accurate_seek_req && !mAbort) {
          int64_t apts = mVideoState->accurate_seek_aframe_pts;
          deviation2 = apts - pts * 1000 * 1000;
          deviation3 = apts - mVideoState->seek_pos;

          if (deviation3 <= 0) {
            usleep(20 * 1000);
          } else {
            break;
          }
          now = CurrentTimeMs();
          if ((now - mVideoState->accurate_seek_start_time) >
              player_config->accurate_seek_timeout) {
            break;
          }
        }

        if (video_seek_pos == mVideoState->seek_pos) {
          mVideoState->drop_vframe_count = 0;
          std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
          mVideoState->video_accurate_seek_req = false;
          mVideoState->audio_accurate_seek_cond.notify_one();
          if (video_seek_pos == mVideoState->seek_pos &&
              mVideoState->audio_accurate_seek_req && !mAbort) {
            mVideoState->video_accurate_seek_cond.wait_for(
                lck, std::chrono::milliseconds(
                         player_config->accurate_seek_timeout));
          } else {
            notifyListener(RED_MSG_ACCURATE_SEEK_COMPLETE,
                           static_cast<int32_t>(pts * 1000));
          }
          if (video_seek_pos != mVideoState->seek_pos && !mAbort) {
            mVideoState->video_accurate_seek_req = true;
            return true;
          }
        }
      }
    } else {
      video_accurate_seek_fail = true;
    }

    if (video_accurate_seek_fail) {
      mVideoState->drop_vframe_count = 0;
      std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
      mVideoState->video_accurate_seek_req = false;
      mVideoState->audio_accurate_seek_cond.notify_one();
      if (mVideoState->audio_accurate_seek_req && !mAbort) {
        mVideoState->video_accurate_seek_cond.wait_for(
            lck,
            std::chrono::milliseconds(player_config->accurate_seek_timeout));
      } else {
        if (!isnan(pts)) {
          notifyListener(RED_MSG_ACCURATE_SEEK_COMPLETE,
                         static_cast<int32_t>(pts * 1000));
        } else {
          notifyListener(RED_MSG_ACCURATE_SEEK_COMPLETE, 0);
        }
      }
    }
    mVideoState->accurate_seek_start_time = 0;
    video_accurate_seek_fail = false;
    mVideoState->accurate_seek_vframe_pts = 0;
  }
  return false;
}

bool CVideoProcesser::pktQueueFrontIsFlush() {
  if (!mRedSourceController)
    return false;
  return mRedSourceController->pktQueueFrontIsFlush(TYPE_VIDEO);
}

RED_ERR CVideoProcesser::getFrame(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (!mFrameQueue) {
    return ME_ERROR;
  }
  if (mFrameQueue->size() <= 0) {
    std::unique_lock<std::mutex> lck(mLock);
    if (mFinishedSerial == mSerial) {
      mVideoState->viddec_finished = true;
      return ME_CLOSED;
    }
  }
  return mFrameQueue->getFrame(buffer);
}

void CVideoProcesser::setConfig(const sp<CoreGeneralConfig> &config) {
  std::lock_guard<std::mutex> lck(mLock);
  mGeneralConfig = config;
}

void CVideoProcesser::setNotifyCb(NotifyCallback notify_cb) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  mNotifyCb = notify_cb;
}

void CVideoProcesser::release() {
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
  if (mFrameQueue) {
    mFrameQueue->flush();
  }
  mPktQueue.clear();
}

void CVideoProcesser::ThreadFunc() {
#if defined(__APPLE__)
  pthread_setname_np("videodec");
#elif __ANDROID__
  pthread_setname_np(pthread_self(), "videodec");
#endif
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (mVideoState->stat.vdec_type == RED_PROPV_DECODER_MEDIACODEC &&
      player_config->mediacodec_auto_rotate) {
    notifyListener(RED_MSG_VIDEO_ROTATION_CHANGED, 0);
  } else {
    notifyListener(RED_MSG_VIDEO_ROTATION_CHANGED,
                   mMetaData->track_info[mMetaData->video_index].rotation);
  }
  while (!mAbort) {
#if defined(__ANDROID__)
    // wait for surface config
    if (mVideoState->stat.vdec_type == RED_PROPV_DECODER_MEDIACODEC) {
      std::unique_lock<std::mutex> lck(mSurfaceLock);
      if (mSurfaceUpdated) {
        mPendingPkt.reset();
        mInputPacketCount = 0;
        if (mFrameQueue) {
          mFrameQueue->flush();
        }
        if (mVideoDecoder) {
          ANativeWindow *window =
              mCurNativeWindow ? mCurNativeWindow->get() : nullptr;
          reddecoder::HardWareContext *hardware_context =
              new reddecoder::AndroidHardWareContext(window);
          mVideoDecoder->update_hardware_context(hardware_context);
        }
        if (mCurNativeWindow && mCurNativeWindow->get()) {
          lck.unlock();
          ResetDecoder();
        }
        mSurfaceUpdated = false;
        continue;
      }
      if ((!mCurNativeWindow || !mCurNativeWindow->get()) && !mAbort) {
        mSurfaceSet.wait_for(lck, std::chrono::milliseconds(10));
        continue;
      }
    }
#endif

    std::unique_ptr<RedAvPacket> pkt;
    RED_ERR ret = OK;
    if (mPendingPkt && !pktQueueFrontIsFlush()) {
      pkt = std::move(mPendingPkt);
    } else {
      ret = ReadPacketOrBuffering(pkt);
    }
    if (ret != OK || !pkt) {
      if (mAbort) {
        continue;
      }
      usleep(20 * 1000);
      continue;
    }
    if (pkt->IsFlushPacket()) {
      PerformFlush();
      mPendingPkt.reset();
      if (mVideoState->stat.vdec_type == RED_PROPV_DECODER_VIDEOTOOLBOX &&
          player_config->vtb_max_error_count) {
        ResetDecoder();
      }
      continue;
    } else if (pkt->IsEofPacket()) {
      AV_LOGI_ID(TAG, mID, "EOF!\n");
      mEOF = true;
      while (!mAbort && !pktQueueFrontIsFlush() && mVideoDecoder &&
             (mInputPacketCount > 0 &&
              mVideoDecoder->get_delayed_frame() ==
                  reddecoder::VideoCodecError::kNoError)) {
      }
      if (pktQueueFrontIsFlush()) {
        continue;
      }
      if (player_config->enable_accurate_seek) {
        std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
        mVideoState->video_accurate_seek_req = false;
        mVideoState->video_accurate_seek_cond.notify_all();
      }
      std::unique_lock<std::mutex> lck(mLock);
      mFinishedSerial = mSerial;
      mFrameQueue->wakeup();
      while (!mAbort && !pktQueueFrontIsFlush()) {
        mCond.wait_for(lck, std::chrono::milliseconds(20));
      }
      continue;
    } else if (pkt->GetSerial() != mSerial) {
      AV_LOGD_ID(TAG, mID, "unequal serial %d-%d\n", pkt->GetSerial(), mSerial);
      continue;
    }

    mEOF = false;
    mFinishedSerial = -1;
    mVideoState->viddec_finished = false;

    if (mVideoState->stat.vdec_type == RED_PROPV_DECODER_VIDEOTOOLBOX &&
        !mPendingPkt) {
      if (pkt->IsKeyPacket() || mPktQueue.size() >= MAX_PKT_QUEUE_DEEP) {
        mIdrBasedIdentified = false;
        mPktQueue.clear();
      }
      std::shared_ptr<RedAvPacket> avpkt(
          new RedAvPacket(pkt->GetAVPacket(), pkt->GetSerial()));
      mPktQueue.emplace_back(avpkt);
    }

    if (mRefreshSession &&
        mVideoState->stat.vdec_type == RED_PROPV_DECODER_VIDEOTOOLBOX) {
      mVideoState->stat.refresh_decoder_count++;
      ResetDecoder();
      DecodeLastCacheGop();
      mRefreshSession = false;
    }
    ret = DecoderTransmit(pkt->GetAVPacket());
    if (ret == ME_RETRY) {
      mPendingPkt = std::move(pkt);
      continue;
    } else {
      mPendingPkt.reset();
      mBuffer.reset();
    }
    if (!mRefreshSession &&
        mVideoState->stat.vdec_type == RED_PROPV_DECODER_VIDEOTOOLBOX &&
        mDecoderErrorCount > player_config->vtb_max_error_count) {
      player_config->videotoolbox = 0;
      RED_ERR ret = Init();
      if (ret != OK) {
        break;
      }
      DecodeLastCacheGop();
    }
    mInputPacketCount++;
    if (!mFirstPacketInDecoder) {
      mFirstPacketInDecoder = true;
      notifyListener(RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER);
    }
  }
  AV_LOGD_ID(TAG, mID, "Video Processer thread Exit.");
}

RED_ERR CVideoProcesser::stop() { return PerformStop(); }

int CVideoProcesser::getSerial() {
  std::unique_lock<std::mutex> lck(mLock);
  return mSerial;
}

void CVideoProcesser::resetEof() {
  std::unique_lock<std::mutex> lck(mLock);
  mEOF = false;
  mFinishedSerial = -1;
  mVideoState->viddec_finished = false;
  if (mFrameQueue) {
    mFrameQueue->flush();
  }
  mCond.notify_one();
}

sp<FrameQueue> CVideoProcesser::frameQueue() {
  std::unique_lock<std::mutex> lck(mLock);
  return mFrameQueue;
}

#if defined(__ANDROID__)
RED_ERR CVideoProcesser::setVideoSurface(const sp<RedNativeWindow> &surface) {
  std::unique_lock<std::mutex> lck(mSurfaceLock);
  if (!surface) {
    return ME_ERROR;
  }
  if (!surface->get()) {
    AV_LOGD_ID(TAG, mID, "set surface null.\n");
  }
  if (surface && mCurNativeWindow &&
      surface->get() == mCurNativeWindow->get()) {
    AV_LOGD_ID(TAG, mID, "set surface is same %p .\n", surface->get());
    return OK;
  }
  AV_LOGD_ID(TAG, mID, "set surface %p .\n", surface->get());
  mPrevNativeWindow = mCurNativeWindow;
  mCurNativeWindow = surface;
  mSurfaceUpdated = true;
  mSurfaceSet.notify_one();
  if (surface && surface->get() && mCodecConfigured) {
    return ME_STATE_ERROR;
  }

  return OK;
}
#endif

REDPLAYER_NS_END;
