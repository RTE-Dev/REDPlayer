/*
 * RedRenderAudioHal.cpp
 *
 *  Created on: 2022年8月19日
 *      Author: liuhongda
 */

#include "RedCore/module/renderHal/RedRenderAudioHal.h"
#include "base/RedConfig.h"

#include "RedMsg.h"

#define TAG "RedRenderAudioHal"

REDPLAYER_NS_BEGIN;

CRedRenderAudioHal::CRedRenderAudioHal(int id, sp<CAudioProcesser> &processer,
                                       const sp<VideoState> &state,
                                       NotifyCallback notify_cb)
    : mID(id), mAudioProcesser(processer), mVideoState(state),
      mNotifyCb(notify_cb) {
  mAudioBuffer = std::make_unique<CGlobalBuffer>();
}

CRedRenderAudioHal::~CRedRenderAudioHal() {
  mAudioRender.reset();
  if (mAudioCallbackClass) {
    mAudioCallbackClass.reset();
  }
  if (mAudioBuffer) {
    mAudioBuffer->free();
  }
  swr_free(&mSwrContext);
  if (mSoundTouchHandle) {
    soundtouchDestroy(&mSoundTouchHandle);
  }
  av_freep(&mAudioNewBuf);
  if (mAudioBuf1Size > 0) {
    av_freep(&mAudioBuf1);
  }
  mAudioProcesser.reset();
  mMetaData.reset();
}

void CRedRenderAudioHal::ThreadFunc() {}

RED_ERR CRedRenderAudioHal::Prepare(sp<MetaData> &metadata) {
  RED_ERR ret = OK;
  mMetaData = metadata;
  mAudioRender = redrender::audio::AudioRenderFactory::CreateAudioRender(mID);

  ret = Init();
  if (ret != OK) {
    notifyListener(RED_MSG_ERROR, ERROR_AUDIO_DISPLAY);
    return ret;
  }

  if (mSoundTouchHandle) {
    soundtouchDestroy(&mSoundTouchHandle);
  }
  mSoundTouchHandle = soundtouchCreate();
  return ret;
}

RED_ERR CRedRenderAudioHal::PerformStart() {
  std::unique_lock<std::mutex> lck(mLock);
  mPaused = false;
  mVideoState->audio_clock->SetClock(mVideoState->audio_clock->GetClock());
  mVideoState->audio_clock->SetPause(false);
  if (mAudioRender) {
    mAudioRender->PauseAudio(0);
  }
  return OK;
}

RED_ERR CRedRenderAudioHal::PerformPause() {
  std::unique_lock<std::mutex> lck(mLock);
  mPaused = true;
  if (mAudioRender) {
    mAudioRender->PauseAudio(1);
  }
  mVideoState->audio_clock->SetPause(true);
  return OK;
}

RED_ERR CRedRenderAudioHal::PerformStop() {
  std::unique_lock<std::mutex> lck(mLock);
  mAbort = true;
  mCond.notify_all();
  return OK;
}

RED_ERR CRedRenderAudioHal::PerformFlush() {
  std::lock_guard<std::mutex> lck(mLock);
  mCond.notify_one();
  return OK;
}

RED_ERR CRedRenderAudioHal::ReadFrame(std::unique_ptr<CGlobalBuffer> &buffer) {
  if (!mAudioProcesser)
    return ME_ERROR;
  return mAudioProcesser->getFrame(buffer);
}

RED_ERR CRedRenderAudioHal::Init() {
  if (!mAudioRender) {
    AV_LOGE_ID(TAG, mID, "%s Failed null render", __func__);
    return NO_INIT;
  }
  if (mPaused) {
    mAudioRender->PauseAudio(1);
  }

  int streamindex = mMetaData->audio_index;

  for (auto iter = mMetaData->track_info.begin();
       iter != mMetaData->track_info.end(); ++iter) {
    if (iter->stream_type == TYPE_AUDIO && iter->stream_index == streamindex) {
      mBytesPerSec =
          av_samples_get_buffer_size(NULL, iter->channels, iter->sample_rate,
                                     AVSampleFormat(iter->sample_fmt), 1);
      // set audio properties
      mDesired.channels = iter->channels;
      mDesired.sample_rate = iter->sample_rate;
      mDesired.channel_layout =
          (iter->channel_layout &&
           iter->channels ==
               av_get_channel_layout_nb_channels(iter->channel_layout))
              ? iter->channel_layout
              : av_get_default_channel_layout(iter->channels);
      mDesired.silence = 0;
      mDesired.format =
          FfmegAudioFmtToRedRenderAudioFmt((AVSampleFormat)iter->sample_fmt);
      mDesired.samples =
          std::max(redrender::audio::KAudioMinBuferSize,
                   2 << av_log2(mDesired.sample_rate /
                                mAudioRender->GetAudioPerSecondCallBacks()));
      AV_LOGI_ID(TAG, mID,
                 "redrender OpenAudio wanted channels:%d, sample_rate:%d, "
                 "channel_layout:%" PRIu64 ", format:%d mBytesPerSec %d\n",
                 mDesired.channels, mDesired.sample_rate,
                 mDesired.channel_layout, mDesired.format, mBytesPerSec);
      break;
    }
  }

  mAudioCallbackClass = std::make_unique<AudioCallbackClass>(
      reinterpret_cast<void *>(this), AudioDataCb);
  int ret = mAudioRender->OpenAudio(mDesired, mObtained, mAudioCallbackClass);
  if (ret < 0) {
    notifyListener(RED_MSG_ERROR, ERROR_AUDIO_DISPLAY,
                   static_cast<int32_t>(ret));
    AV_LOGE_ID(TAG, mID, "redrender OpenAudio error:%d", ret);
    return ME_ERROR;
  }

  if (mObtained.channels != mDesired.channels) {
    mObtained.channel_layout =
        av_get_default_channel_layout(mObtained.channels);
    if (!mObtained.channel_layout) {
      AV_LOGE_ID(TAG, mID, "channel count %d is not support",
                 mObtained.channels);
      return ME_ERROR;
    }
  }
  mBytesPerSec = av_samples_get_buffer_size(
      NULL, mObtained.channels, mObtained.sample_rate,
      RedRenderAudioFmtToFfmegAudioFmt(mObtained.format), 1);
  mAudioRender->SetDefaultLatencySeconds(
      (static_cast<double>(2 * mObtained.size)) / mBytesPerSec);
  mAudioDelay = mAudioRender->GetLatencySeconds();
  mCurrentInfo = mObtained;

  AV_LOGI_ID(
      TAG, mID,
      "redrender OpenAudio obtained channels:%d, sample_rate:%d, "
      "channel_layout:%" PRIu64 ", format:%d mAudioDelay %f, mBytesPerSec %d\n",
      mObtained.channels, mObtained.sample_rate, mObtained.channel_layout,
      mObtained.format, mAudioDelay, mBytesPerSec);
  return OK;
}

int CRedRenderAudioHal::ResampleAudioData(
    std::unique_ptr<CGlobalBuffer> &buffer) {
  int data_size = 0;
  int resampled_data_size = 0;
  int64_t dec_channel_layout;
  int wanted_nb_samples;
  if (!buffer->channel[0]) {
    AV_LOGE_ID(TAG, mID, "Invalid audio data\n");
    return ME_RETRY;
  }
  data_size =
      av_samples_get_buffer_size(NULL, buffer->num_channels, buffer->nb_samples,
                                 AVSampleFormat(buffer->format), 1);
  dec_channel_layout =
      (buffer->channel_layout &&
       buffer->num_channels ==
           av_get_channel_layout_nb_channels(buffer->channel_layout))
          ? buffer->channel_layout
          : av_get_default_channel_layout(buffer->num_channels);
  wanted_nb_samples = buffer->nb_samples;
  if (FfmegAudioFmtToRedRenderAudioFmt((AVSampleFormat)buffer->format) !=
          mCurrentInfo.format ||
      dec_channel_layout != mCurrentInfo.channel_layout ||
      buffer->sample_rate != mCurrentInfo.sample_rate ||
      (wanted_nb_samples != buffer->nb_samples && !mSwrContext)) {
    AVDictionary *swr_opts = NULL;
    swr_free(&mSwrContext);
    mSwrContext = swr_alloc_set_opts(
        NULL, mObtained.channel_layout,
        RedRenderAudioFmtToFfmegAudioFmt(mObtained.format),
        mObtained.sample_rate, dec_channel_layout,
        (enum AVSampleFormat)buffer->format, buffer->sample_rate, 0, NULL);
    if (!mSwrContext) {
      AV_LOGE_ID(TAG, mID,
                 "Cannot create sample rate converter for conversion of %d Hz "
                 "%s %d channels to %d Hz %s %d channels!\n",
                 buffer->sample_rate,
                 av_get_sample_fmt_name((enum AVSampleFormat)buffer->format),
                 buffer->num_channels, mObtained.sample_rate,
                 av_get_sample_fmt_name(
                     RedRenderAudioFmtToFfmegAudioFmt(mObtained.format)),
                 mObtained.channels);
      return ME_ERROR;
    }

    av_opt_set_dict(mSwrContext, &swr_opts);
    av_dict_free(&swr_opts);

    if (swr_init(mSwrContext) < 0) {
      AV_LOGE_ID(TAG, mID,
                 "Cannot create sample rate converter for conversion of %d Hz "
                 "%s %d channels to %d Hz %s %d channels!\n",
                 buffer->sample_rate,
                 av_get_sample_fmt_name((enum AVSampleFormat)buffer->format),
                 buffer->num_channels, mObtained.sample_rate,
                 av_get_sample_fmt_name(
                     RedRenderAudioFmtToFfmegAudioFmt(mObtained.format)),
                 mObtained.channels);
      swr_free(&mSwrContext);
      return ME_ERROR;
    }

    AV_LOGD_ID(TAG, mID,
               "Success create sample rate converter %p for conversion of %d "
               "Hz %s %d channels to %d Hz %s %d channels!\n",
               mSwrContext, buffer->sample_rate,
               av_get_sample_fmt_name((enum AVSampleFormat)buffer->format),
               buffer->num_channels, mObtained.sample_rate,
               av_get_sample_fmt_name(
                   RedRenderAudioFmtToFfmegAudioFmt(mObtained.format)),
               mObtained.channels);

    mCurrentInfo.channel_layout = dec_channel_layout;
    mCurrentInfo.channels = buffer->num_channels;
    mCurrentInfo.sample_rate = buffer->sample_rate;
    mCurrentInfo.format =
        FfmegAudioFmtToRedRenderAudioFmt((AVSampleFormat)buffer->format);
  }
  if (mSwrContext) {
    const uint8_t **in = (const uint8_t **)&buffer->channel;
    uint8_t **out = &mAudioBuf1;
    int out_count =
        static_cast<int>(static_cast<int64_t>(wanted_nb_samples) *
                             mObtained.sample_rate / buffer->sample_rate +
                         256);
    int out_size = av_samples_get_buffer_size(
        NULL, mObtained.channels, out_count,
        RedRenderAudioFmtToFfmegAudioFmt(mObtained.format), 0);
    int len2;
    if (out_size < 0) {
      AV_LOGE_ID(TAG, mID, "av_samples_get_buffer_size() failed\n");
      return ME_ERROR;
    }
    if (wanted_nb_samples != buffer->nb_samples) {
      if (swr_set_compensation(mSwrContext,
                               (wanted_nb_samples - buffer->nb_samples) *
                                   mObtained.sample_rate / buffer->sample_rate,
                               wanted_nb_samples * mObtained.sample_rate /
                                   buffer->sample_rate) < 0) {
        AV_LOGE_ID(TAG, mID, "swr_set_compensation() failed\n");
        return ME_ERROR;
      }
    }
    av_fast_malloc(&mAudioBuf1, &mAudioBuf1Size, out_size);

    if (!mAudioBuf1) {
      AV_LOGE_ID(TAG, mID, "create mAudioBuf1 failed\n");
      return ME_ERROR;
    }

    len2 = swr_convert(mSwrContext, out, out_count, in, buffer->nb_samples);
    if (len2 < 0) {
      AV_LOGE_ID(TAG, mID, "swr_convert() failed\n");
      return ME_ERROR;
    }
    if (len2 == out_count) {
      AV_LOGW_ID(TAG, mID, "audio buffer is probably too small\n");
      if (swr_init(mSwrContext) < 0)
        swr_free(&mSwrContext);
    }
    mAudioBuf = mAudioBuf1;
    int bytes_per_sample = av_get_bytes_per_sample(
        RedRenderAudioFmtToFfmegAudioFmt(mObtained.format));
    resampled_data_size = len2 * mObtained.channels * bytes_per_sample;
    mBytesPerSec =
        buffer->sample_rate * buffer->num_channels * bytes_per_sample;
  } else {
    mAudioBuf = buffer->channel[0];
    mAudioBuf1 = mAudioBuf;
    resampled_data_size = data_size;
  }
  if (!mFirstFrameDecoded) {
    mFirstFrameDecoded = true;
    notifyListener(RED_MSG_AUDIO_DECODED_START);
  }
  return resampled_data_size;
}

void CRedRenderAudioHal::GetAudioData(char *data, int &len) {
  if (!data || len <= 0) {
    return;
  }
  std::unique_lock<std::mutex> lck(mLock);
  int left_size = len;
  int offset = 0;
  double current_pts = 0;
  memset(data, 0, len);
  int translate_time = 1;

  if (mAbort) {
    return;
  }

  if (mVolumeChanged) {
    mVolumeChanged = false;
    if (mAudioRender) {
      AV_LOGI_ID(TAG, mID, "volume changed %f %f\n", mLeftVolume, mRightVolume);
#if defined(__ANDROID__)
      mAudioRender->SetStereoVolume(mLeftVolume, mRightVolume);
#elif __APPLE__
      mAudioRender->SetPlaybackVolume((mLeftVolume + mRightVolume) / 2);
#endif
    }
  }

  while (left_size > 0 && !mAbort) {
    if (mLastReadPos == 0) {
    reload:
      mAudioBuffer->free();
      mAudioBufSize = 0;
      RED_ERR ret = OK;
      std::unique_ptr<CGlobalBuffer> buffer;
      lck.unlock();
      ret = ReadFrame(buffer);
      if (ret != OK || !buffer) {
        return;
      }
      int resampled_data_size = ResampleAudioData(buffer);
      if (resampled_data_size <= 0) {
        AV_LOGW_ID(TAG, mID, "Failed to resample audio data!\n");
        return;
      }

      if (mPlaybackRate != 1.0f && !mAbort) {
        int wanted_nb_samples = buffer->nb_samples;
        int out_count =
            static_cast<int>(static_cast<int64_t>(wanted_nb_samples) *
                                 mObtained.sample_rate / buffer->sample_rate +
                             256);
        int out_size = av_samples_get_buffer_size(
            nullptr, mObtained.channels, out_count,
            RedRenderAudioFmtToFfmegAudioFmt(mObtained.format), 0);
        int bytes_per_sample = av_get_bytes_per_sample(
            RedRenderAudioFmtToFfmegAudioFmt(mObtained.format));
        int min_times = 1;
        if (mPlaybackRate > 0 && mPlaybackRate < 1.0f) {
          min_times = static_cast<int>(ceil(1.0 / mPlaybackRate));
        }
        av_fast_malloc(&mAudioNewBuf, &mAudioNewBufSize,
                       out_size * translate_time * min_times);
        for (int i = 0; i < (resampled_data_size / 2); i++) {
          mAudioNewBuf[i] = (mAudioBuf1[i * 2] | (mAudioBuf1[i * 2 + 1] << 8));
        }
        int ret_len = soundtouchTranslate(
            mSoundTouchHandle, mAudioNewBuf, static_cast<float>(mPlaybackRate),
            static_cast<float>(1.0f) / mPlaybackRate, resampled_data_size / 2,
            bytes_per_sample, mObtained.channels, buffer->sample_rate);
        if (ret_len > 0) {
          mAudioBuf = reinterpret_cast<uint8_t *>(mAudioNewBuf);
          resampled_data_size = ret_len;
        } else {
          translate_time++;
          lck.lock();
          goto reload;
        }
      }
      lck.lock();
      mAudioBuffer = std::move(buffer);
      mAudioBufSize = resampled_data_size;
    }

    if (mAudioBuffer->serial != mAudioProcesser->getSerial()) {
      mLastReadPos = 0;
      mAudioBufSize = 0;
      if (mAudioRender) {
        mAudioRender->FlushAudio();
      }
      return;
    }
    int write_size = std::min(left_size, mAudioBufSize - mLastReadPos);
    if (mMute.load()) {
      memset(data + offset, 0, write_size);
    } else {
      memcpy(data + offset, mAudioBuf + mLastReadPos, write_size);
    }
    mLastReadPos += write_size;
    offset += write_size;
    left_size -= write_size;
    if (mBytesPerSec > 0)
      current_pts = mAudioBuffer->pts +
                    static_cast<double>(mLastReadPos) / mBytesPerSec * 1000;
    else
      current_pts = mAudioBuffer->pts;
    mLastReadPos = mLastReadPos == mAudioBufSize ? 0 : mLastReadPos;
  }
  lck.unlock();
  if (current_pts > 0) {
    mVideoState->audio_clock->SetClock(current_pts / 1000.0 - mAudioDelay);
    if (mVideoState->audio_clock->GetClockSerial() != mAudioBuffer->serial) {
      mVideoState->audio_clock->SetClockSerial(mAudioBuffer->serial);
    }
    if (!mVideoState->audio_clock->GetClockAvaliable()) {
      mVideoState->audio_clock->SetClockAvaliable(true);
    }
  }
  if (!mVideoState->first_audio_frame_rendered) {
    mVideoState->first_audio_frame_rendered = 1;
    notifyListener(RED_MSG_AUDIO_RENDERING_START);
  }

  if (mAudioBuffer->serial >= 0 &&
      mVideoState->latest_audio_seek_load_serial == mAudioBuffer->serial) {
    int latest_audio_seek_load_serial =
        __atomic_exchange_n(&(mVideoState->latest_audio_seek_load_serial), -1,
                            std::memory_order_seq_cst);
    if (latest_audio_seek_load_serial == mAudioBuffer->serial) {
      if (getMasterSyncType(mVideoState) == CLOCK_AUDIO) {
        notifyListener(RED_MSG_AUDIO_SEEK_RENDERING_START, 1);
      } else {
        notifyListener(RED_MSG_AUDIO_SEEK_RENDERING_START, 0);
      }
    }
  }
  return;
}

void CRedRenderAudioHal::AudioDataCb(void *userdata, char *data, int &len) {
  CRedRenderAudioHal *thiz = reinterpret_cast<CRedRenderAudioHal *>(userdata);
  thiz->GetAudioData(data, len);
}

void CRedRenderAudioHal::notifyListener(uint32_t what, int32_t arg1,
                                        int32_t arg2, void *obj1, void *obj2,
                                        int obj1_len, int obj2_len) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  if (mNotifyCb) {
    mNotifyCb(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

void CRedRenderAudioHal::setConfig(const sp<CoreGeneralConfig> &config) {
  std::lock_guard<std::mutex> lck(mLock);
  mGeneralConfig = config;
}

void CRedRenderAudioHal::setNotifyCb(NotifyCallback notify_cb) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  mNotifyCb = notify_cb;
}

void CRedRenderAudioHal::release() {
  AV_LOGD_ID(TAG, mID, "%s start\n", __func__);
  if (mAudioRender) {
    mAudioRender->CloseAudio();
    mAudioRender->WaitClose();
  }
}

float CRedRenderAudioHal::getPlaybackRate() { return mPlaybackRate; }

void CRedRenderAudioHal::setPlaybackRate(const float rate) {
  AV_LOGI_ID(TAG, mID, "%s %f\n", __func__, rate);
  if (std::abs(rate - 0.0) > FLT_EPSILON &&
      std::abs(mPlaybackRate - rate) > FLT_EPSILON) {
    mPlaybackRate = rate;
    mPlaybackRateChanged = true;
  }
}

float CRedRenderAudioHal::getVolume() {
  return (mLeftVolume + mRightVolume) / 2;
}

void CRedRenderAudioHal::setVolume(const float left_volume,
                                   const float right_volume) {
  AV_LOGI_ID(TAG, mID, "%s %f %f\n", __func__, left_volume, right_volume);
  if (std::abs(mLeftVolume - left_volume) < FLT_EPSILON &&
      std::abs(mRightVolume - right_volume) < FLT_EPSILON) {
    return;
  }
  mLeftVolume = std::min(std::max(left_volume, 0.0f), 1.0f);
  mRightVolume = std::min(std::max(right_volume, 0.0f), 1.0f);
  mVolumeChanged = true;
}

void CRedRenderAudioHal::setMute(bool mute) {
  AV_LOGI_ID(TAG, mID, "%s %d\n", __func__, mute);
  mMute.store(mute);
}

RED_ERR CRedRenderAudioHal::pause() { return PerformPause(); }

RED_ERR CRedRenderAudioHal::start() { return PerformStart(); }

RED_ERR CRedRenderAudioHal::stop() { return PerformStop(); }

RED_ERR CRedRenderAudioHal::flush() { return PerformFlush(); }

redrender::audio::AudioFormat
CRedRenderAudioHal::FfmegAudioFmtToRedRenderAudioFmt(
    enum AVSampleFormat sampleFmt) {
  enum AVSampleFormat packedSampleFmt = av_get_packed_sample_fmt(sampleFmt);
  switch (packedSampleFmt) {
  case AV_SAMPLE_FMT_U8:
    return redrender::audio::AudioFormat::kAudioU8;
  case AV_SAMPLE_FMT_S32:
    return redrender::audio::AudioFormat::kAudioS32Sys;
  case AV_SAMPLE_FMT_FLT:
  case AV_SAMPLE_FMT_DBL:
    return redrender::audio::AudioFormat::kAudioF32Sys;
  default:
    break;
  }

  return redrender::audio::AudioFormat::kAudioS16Sys;
}

AVSampleFormat CRedRenderAudioHal::RedRenderAudioFmtToFfmegAudioFmt(
    redrender::audio::AudioFormat fmt) {
  switch (fmt) {
  case redrender::audio::AudioFormat::kAudioS32Sys:
    return AV_SAMPLE_FMT_S32;
  case redrender::audio::AudioFormat::kAudioF32Sys:
    return AV_SAMPLE_FMT_FLT;
  case redrender::audio::AudioFormat::kAudioU8:
    return AV_SAMPLE_FMT_U8;
  default:
    break;
  }
  return AV_SAMPLE_FMT_S16;
}

REDPLAYER_NS_END;
