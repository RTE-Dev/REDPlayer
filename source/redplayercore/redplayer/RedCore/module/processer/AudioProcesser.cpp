/*
 * AudioProcesser.cpp
 *
 *  Created on: 2022年8月19日
 *      Author: liuhongda
 */

#include "AudioProcesser.h"
#include "RedMsg.h"
#include "base/RedConfig.h"
#include "reddecoder/audio/audio_decoder/audio_decoder_factory.h"

#define TAG "AudioProcesser"

REDPLAYER_NS_BEGIN;

CAudioProcesser::CAudioProcesser(int id,
                                 sp<CRedSourceController> &sourceManager,
                                 const sp<VideoState> &state,
                                 NotifyCallback notify_cb)
    : mID(id), mRedSourceController(sourceManager), mVideoState(state),
      mNotifyCb(notify_cb) {}

CAudioProcesser::~CAudioProcesser() {
  mFrameQueue.reset();
  mAudioDecoder.reset();
  mRedSourceController.reset();
  mMetaData.reset();
}

RED_ERR CAudioProcesser::Prepare(const sp<MetaData> &metadata) {
  RED_ERR ret = OK;
  mMetaData = metadata;
  ret = Init();
  if (ret != OK) {
    notifyListener(RED_MSG_ERROR, ERROR_STREAM_OPEN);
    return ret;
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

RED_ERR CAudioProcesser::Init() {
  if (!mMetaData) {
    return ME_RETRY;
  }
  std::string codec_module, codec_name;
  auto track_info = mMetaData->track_info[mMetaData->audio_index];
  std::unique_ptr<reddecoder::AudioDecoderFactory> decoder_factory =
      std::make_unique<reddecoder::AudioDecoderFactory>();

  reddecoder::AudioCodecName audio_codec_name =
      reddecoder::AudioCodecName::kOpaque;
  reddecoder::AudioCodecProfile audio_profile =
      reddecoder::AudioCodecProfile::kUnknown;

  reddecoder::AudioCodecInfo codec_info(
      audio_codec_name, audio_profile,
      reddecoder::AudioCodecImplementationType::kSoftware);
  codec_info.opaque_codec_id = track_info.codec_id;
  codec_info.opaque_profile = track_info.codec_profile;

  mAudioDecoder = decoder_factory->create_audio_decoder(codec_info);
  if (!mAudioDecoder) {
    AV_LOGE_ID(TAG, mID, "Audio decoder create error");
    notifyListener(RED_MSG_ERROR, ERROR_DECODER_ADEC,
                   -(static_cast<int32_t>(DECODER_INIT_ERROR)));
    return ME_ERROR;
  }
  mAudioDecoder->register_decode_complete_callback(this);
  ResetDecoderFormat();

  mFrameQueue = std::make_unique<FrameQueue>(SAMPLE_QUEUE_SIZE, TYPE_AUDIO);
  if (!mFrameQueue) {
    AV_LOGE_ID(TAG, mID, "Audio frame queue create error\n");
    return ME_ERROR;
  }

  codec_name = avcodec_get_name((AVCodecID)track_info.codec_id);
  codec_module = AVCODEC_MODULE_NAME;
  mVideoState->audio_codec_info = codec_module + ", " + codec_name;

  AV_LOGI_ID(TAG, mID, "Audio decoder create success, codec name %s\n",
             mVideoState->audio_codec_info.c_str());
  return OK;
}

reddecoder::AudioCodecError CAudioProcesser::on_decoded_frame(
    std::unique_ptr<reddecoder::Buffer> decoded_frame) {

  auto meta = decoded_frame->get_audio_frame_meta();
  std::unique_ptr<CGlobalBuffer> buffer(new CGlobalBuffer());
  if (!buffer) {
    return reddecoder::AudioCodecError::kInitError;
  }
  buffer->pixel_format = CGlobalBuffer::kAudio;
  buffer->data = decoded_frame->get_away_data();
  buffer->datasize = decoded_frame->get_size();
  buffer->pts = meta->pts_ms;
  buffer->num_channels = meta->num_channels;
  buffer->nb_samples = meta->num_samples;
  for (int i = 0; i < meta->num_channels; i++) {
    buffer->channel[i] = meta->channel[i];
  }
  buffer->channel_layout = meta->channel_layout;
  buffer->sample_rate = meta->sample_rate ? meta->sample_rate : 44100;
  buffer->format = meta->sample_format;
  buffer->serial = mSerial;

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

  if (!mFrameQueue) {
    return reddecoder::AudioCodecError::kInitError;
  }

  if (mAbort) {
    return reddecoder::AudioCodecError::kNoError;
  }

  if (checkAccurateSeek(buffer)) {
    return reddecoder::AudioCodecError::kNoError;
  }
  mFrameQueue->putFrame(buffer);
  return reddecoder::AudioCodecError::kNoError;
}

void CAudioProcesser::on_decode_error(reddecoder::AudioCodecError error) {}

RED_ERR CAudioProcesser::DecoderTransmit(AVPacket *pkt) {
  RED_ERR ret = OK;
  if (!mAudioDecoder)
    return ME_ERROR;

  reddecoder::Buffer buffer(reddecoder::BufferType::kAudioPacket, pkt->data,
                            pkt->size, false);
  auto meta = buffer.get_audio_packet_meta();
  AVRational ms_time_base = {1, 1000};
  auto track_info = mMetaData->track_info[mMetaData->audio_index];
  AVRational origin_time_base =
      (AVRational){track_info.time_base_num, track_info.time_base_den};
  meta->pts_ms = av_rescale_q(pkt->pts, origin_time_base, ms_time_base);
  auto err = mAudioDecoder->decode(&buffer);
  if (err != reddecoder::AudioCodecError::kNoError) {
    return ME_ERROR;
  }
  return ret;
}

RED_ERR
CAudioProcesser::ReadPacketOrBuffering(std::unique_ptr<RedAvPacket> &pkt) {
  if (!mRedSourceController)
    return ME_ERROR;
  RED_ERR ret = mRedSourceController->getPacket(pkt, TYPE_AUDIO, false);
  if (ret == ME_RETRY && !mEOF) {
    if (mVideoState->first_audio_frame_rendered) {
      mRedSourceController->toggleBuffering(true);
    }
    ret = mRedSourceController->getPacket(pkt, TYPE_AUDIO, true);
  }
  return ret;
}

RED_ERR CAudioProcesser::PerformStop() {
  std::unique_lock<std::mutex> lck(mLock);
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (player_config->enable_accurate_seek) {
    std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
    mVideoState->audio_accurate_seek_req = false;
    mVideoState->audio_accurate_seek_cond.notify_all();
  }
  mAbort = true;
  mCond.notify_all();
  if (mFrameQueue) {
    mFrameQueue->abort();
  }
  return OK;
}

RED_ERR CAudioProcesser::PerformFlush() {
  std::lock_guard<std::mutex> lck(mLock);
  RED_ERR ret = OK;
  mSerial++;
  mEOF = false;
  if (mAudioDecoder) {
    mAudioDecoder->flush();
  }
  if (mFrameQueue) {
    mFrameQueue->flush();
  }
  AV_LOGI_ID(TAG, mID, "%s serial %d\n", __func__, mSerial);
  return ret;
}

RED_ERR CAudioProcesser::ResetDecoderFormat() {
  RED_ERR ret = OK;
  if (!mMetaData) {
    return ME_RETRY;
  }
  if (!mAudioDecoder)
    return ME_ERROR;
  auto track_info = mMetaData->track_info[mMetaData->audio_index];
  reddecoder::AudioCodecConfig config;
  config.channels = track_info.channels;
  config.sample_rate = track_info.sample_rate;
  config.extradata = track_info.extra_data;
  config.extradata_size = track_info.extra_data_size;
  mAudioDecoder->init(config);
  return ret;
}

void CAudioProcesser::notifyListener(uint32_t what, int32_t arg1, int32_t arg2,
                                     void *obj1, void *obj2, int obj1_len,
                                     int obj2_len) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  if (mNotifyCb) {
    mNotifyCb(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

bool CAudioProcesser::checkAccurateSeek(
    const std::unique_ptr<CGlobalBuffer> &buffer) {
  bool audio_accurate_seek_fail = false;
  int64_t audio_seek_pos = 0;
  int64_t now = 0;
  int64_t deviation = 0;
  int64_t deviation3 = 0;
  double frame_pts = buffer->pts / 1000;
  double samples_duration =
      static_cast<double>(buffer->nb_samples) / buffer->sample_rate;
  double audio_clock = 0;
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (player_config->enable_accurate_seek &&
      mVideoState->audio_accurate_seek_req && !mVideoState->seek_req) {
    now = CurrentTimeMs();
    if (!isnan(frame_pts)) {
      audio_clock = frame_pts + samples_duration;
      mVideoState->accurate_seek_aframe_pts = audio_clock * 1000 * 1000;
      audio_seek_pos = mVideoState->seek_pos;
      deviation =
          llabs((int64_t)(audio_clock * 1000 * 1000) - mVideoState->seek_pos);
      if ((audio_clock * 1000 * 1000 < mVideoState->seek_pos) ||
          deviation < MAX_DEVIATION) {
        if (mVideoState->drop_aframe_count == 0) {
          std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
          if (mVideoState->accurate_seek_start_time <= 0 &&
              (mMetaData->video_index < 0 ||
               mVideoState->video_accurate_seek_req)) {
            mVideoState->accurate_seek_start_time = now;
          }
        }
        mVideoState->drop_aframe_count++;
        if (!mVideoState->video_accurate_seek_req &&
            mMetaData->video_index >= 0 &&
            audio_clock * 1000 * 1000 > mVideoState->accurate_seek_vframe_pts) {
          audio_accurate_seek_fail = true;
        } else {
          now = CurrentTimeMs();
          if ((now - mVideoState->accurate_seek_start_time) <=
              player_config->accurate_seek_timeout) {
            return true; // drop some old frame when do accurate seek
          } else {
            audio_accurate_seek_fail = true;
          }
        }
      } else {
        while (mVideoState->video_accurate_seek_req && !mAbort) {
          int64_t vpts = mVideoState->accurate_seek_vframe_pts;
          deviation3 = vpts - mVideoState->seek_pos;
          if (deviation3 >= 0) {
            break;
          } else {
            usleep(20 * 1000);
          }
          now = CurrentTimeMs();
          if ((now - mVideoState->accurate_seek_start_time) >
              player_config->accurate_seek_timeout) {
            break;
          }
          if (audio_seek_pos != mVideoState->seek_pos) {
            break;
          }
        }
        if (audio_seek_pos == mVideoState->seek_pos) {
          mVideoState->drop_aframe_count = 0;
          std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
          mVideoState->audio_accurate_seek_req = false;
          mVideoState->video_accurate_seek_cond.notify_one();
          if (audio_seek_pos == mVideoState->seek_pos &&
              mVideoState->video_accurate_seek_req && !mAbort) {
            mVideoState->audio_accurate_seek_cond.wait_for(
                lck, std::chrono::milliseconds(
                         player_config->accurate_seek_timeout));
          } else {
            notifyListener(RED_MSG_ACCURATE_SEEK_COMPLETE,
                           static_cast<int32_t>(audio_clock * 1000));
          }

          if (audio_seek_pos != mVideoState->seek_pos && !mAbort) {
            mVideoState->audio_accurate_seek_req = true;
            return true;
          }
        }
      }
    } else {
      audio_accurate_seek_fail = true;
    }
    if (audio_accurate_seek_fail) {
      mVideoState->drop_aframe_count = 0;
      std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
      mVideoState->audio_accurate_seek_req = false;
      mVideoState->video_accurate_seek_cond.notify_one();
      if (mVideoState->video_accurate_seek_req && !mAbort) {
        mVideoState->audio_accurate_seek_cond.wait_for(
            lck,
            std::chrono::milliseconds(player_config->accurate_seek_timeout));
      } else {
        notifyListener(RED_MSG_ACCURATE_SEEK_COMPLETE,
                       static_cast<int32_t>(audio_clock * 1000));
      }
    }
    mVideoState->accurate_seek_start_time = 0;
    audio_accurate_seek_fail = false;
  }
  return false;
}

RED_ERR CAudioProcesser::getFrame(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (!mFrameQueue) {
    return ME_ERROR;
  }
  if (mFrameQueue->size() <= 0) {
    std::unique_lock<std::mutex> lck(mLock);
    if (mEOF) {
      mVideoState->auddec_finished = true;
      return ME_CLOSED;
    }
  }
  return mFrameQueue->getFrame(buffer);
}

void CAudioProcesser::setConfig(const sp<CoreGeneralConfig> &config) {
  std::lock_guard<std::mutex> lck(mLock);
  mGeneralConfig = config;
}

void CAudioProcesser::setNotifyCb(NotifyCallback notify_cb) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  mNotifyCb = notify_cb;
}

void CAudioProcesser::release() {
  AV_LOGD_ID(TAG, mID, "%s start\n", __func__);
  mAbort = true;
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
}

// base method
void CAudioProcesser::ThreadFunc() {
#if defined(__APPLE__)
  pthread_setname_np("audiodec");
#elif defined(__ANDROID__) || defined(__HARMONY__)
  pthread_setname_np(pthread_self(), "audiodec");
#endif
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  while (!mAbort) {
    std::unique_ptr<RedAvPacket> pkt;
    RED_ERR ret = ReadPacketOrBuffering(pkt);
    if (ret != OK || !pkt) {
      if (mAbort) {
        continue;
      }
      usleep(20 * 1000);
      continue;
    }
    if (pkt->IsFlushPacket()) {
      PerformFlush();
      continue;
    } else if (pkt->IsEofPacket()) {
      AV_LOGI_ID(TAG, mID, "EOF!\n");
      if (player_config->enable_accurate_seek) {
        std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
        mVideoState->audio_accurate_seek_req = false;
        mVideoState->audio_accurate_seek_cond.notify_all();
      }
      std::unique_lock<std::mutex> lck(mLock);
      mEOF = true;
      mFrameQueue->wakeup();
      if (!mAbort) {
        mCond.wait(lck);
      }
      continue;
    } else if (pkt->GetSerial() != mSerial) {
      continue;
    }
    mVideoState->auddec_finished = false;
    DecoderTransmit(pkt->GetAVPacket());
  }
  AV_LOGD_ID(TAG, mID, "Audio Processer thread exit.");
}

RED_ERR CAudioProcesser::stop() {
  PerformStop();
  return OK;
}

int CAudioProcesser::getSerial() {
  std::unique_lock<std::mutex> lck(mLock);
  return mSerial;
}

void CAudioProcesser::resetEof() {
  std::unique_lock<std::mutex> lck(mLock);
  mEOF = false;
  mVideoState->auddec_finished = false;
  if (mFrameQueue) {
    mFrameQueue->flush();
  }
  mCond.notify_one();
}

REDPLAYER_NS_END;
