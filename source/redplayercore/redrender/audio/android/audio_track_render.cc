#if defined(__ANDROID__)
#include "./audio_track_render.h"

#include "../audio_common.h"
#include "./audio_track_common.h"
#include "jni/Env.h"
#include "jni/Util.h"
USING_NS_REDRENDER_AUDIO
AudioTrackRender::AudioTrackRender() : AudioTrackRender(0) {}

AudioTrackRender::AudioTrackRender(const int &session_id)
    : IAudioRender(session_id) {}

AudioTrackRender::~AudioTrackRender() {
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_,
             "[%s:%d] AudioTrackRender Deconstruct start\n", __FUNCTION__,
             __LINE__);
  if (buffer_) {
    delete[] buffer_;
    buffer_ = nullptr;
  }
  jni::JniEnvPtr jni_env;
  JNIEnv *env = jni_env.env();
  if (!env) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] env is null!\n",
               __FUNCTION__, __LINE__);
    return;
  }
  if (byte_buffer_) {
    JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&byte_buffer_));
  }
  byte_buffer_capacity_ = 0;
  if (audio_track_) {
    AudioTrackJni::AudioTrackReleaseCatchAll(env, audio_track_);
    JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&audio_track_));
  }
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_,
             "[%s:%d] AudioTrackRender Deconstruct end\n", __FUNCTION__,
             __LINE__);
}
void AudioTrackRender::AdjustAudioInfo(const AudioInfo &desired,
                                       AudioInfo &obtained) {
  obtained = desired;
  if (desired.sample_rate < 4000 || desired.sample_rate > 48000) {
    obtained.sample_rate = 44100;
  }
  if (desired.channels > 2) {
    obtained.channels = 2;
  }
  if (obtained.format != AudioFormat::kAudioS16Sys) {
    obtained.format = AudioFormat::kAudioS16Sys;
  }
}

int AudioTrackRender::OpenAudio(
    const AudioInfo &desired, AudioInfo &obtained,
    std::unique_ptr<AudioCallback> &audio_callback) {
  AdjustAudioInfo(desired, obtained);
  int stream_type = static_cast<int>(AudioTrackStreamType::kStreamMusic);
  int audio_format = static_cast<int>(AudioTrackFormat::kEncodingPcm16Bit);
  int channel_config = 0;
  if (obtained.channels == 2) {
    channel_config =
        static_cast<int>(AudioTrackChannelConfig::kChannelOutStero);
  } else if (obtained.channels == 1) {
    channel_config = static_cast<int>(AudioTrackChannelConfig::kChannelOutMono);
  } else {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] channels = %d,not support!\n", __FUNCTION__, __LINE__,
               obtained.channels);
    return -1;
  }
  int sample_rate_in_hz = obtained.sample_rate;
  int mode = static_cast<int>(AudioTrackMode::kModeStream);
  jni::JniEnvPtr jni_env;
  JNIEnv *env = jni_env.env();
  if (!env) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] env is null!\n",
               __FUNCTION__, __LINE__);
    return -1;
  }
  int buffer_size = AudioTrackJni::AudioTrackGetMinBufferSizeCatchAll(
      env, sample_rate_in_hz, channel_config, audio_format);

  if (buffer_size <= 0) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] buffer_size is %d, failed!\n", __FUNCTION__, __LINE__,
               buffer_size);
    return -1;
  }

  buffer_size *= KAudioTrackPlaybackMaxSpeed;

  audio_track_ = AudioTrackJni::AudioTrackAsGlobalRefCatchAll(
      env, stream_type, sample_rate_in_hz, channel_config, audio_format,
      buffer_size, mode);
  audio_session_id_ =
      AudioTrackJni::AudioTrackGetAudioSessionIdCatchAll(env, audio_track_);
  buffer_size_ = buffer_size;
  obtained.size = buffer_size;
  buffer_ = new uint8_t[buffer_size];
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] buffer_size is %d!\n",
             __FUNCTION__, __LINE__, buffer_size);
  audio_callback_ = std::move(audio_callback);
  std::unique_lock<std::mutex> mutex(thread_mutex_);
  if (!abort_request_ && !aout_thread_) {
    try {
      aout_thread_ = new std::thread(audio_loop, this);
    } catch (const std::system_error &e) {
      aout_thread_ = nullptr;
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] Exception caught: %s!\n",
                 __FUNCTION__, __LINE__, e.what());
      return -1;
    }
  }
  return 0;
}

int AudioTrackRender::audio_loop(AudioTrackRender *atr) {
  int copy_size = 256;
  jni::JniEnvPtr jni_env;
  JNIEnv *env = jni_env.env();
  if (!atr->abort_request_ && !atr->pause_on_)
    AudioTrackJni::AudioTrackPlayCatchAll(env, atr->audio_track_);
  while (!atr->abort_request_) {
    {
      std::unique_lock<std::mutex> mutex(atr->mutex_);
      if (!atr->abort_request_ && atr->pause_on_) {
        AudioTrackJni::AudioTrackPauseCatchAll(env, atr->audio_track_);
        while (!atr->abort_request_ && atr->pause_on_) {
          atr->wakeup_cond_.wait_for(mutex, std::chrono::milliseconds(1000));
        }
        if (!atr->abort_request_ && !atr->pause_on_) {
          if (atr->need_flush_) {
            atr->need_flush_ = 0;
            AudioTrackJni::AudioTrackFlushCatchAll(env, atr->audio_track_);
          }
          AudioTrackJni::AudioTrackPlayCatchAll(env, atr->audio_track_);
        } else if (atr->abort_request_) {
          break;
        }
      }

      if (atr->need_flush_) {
        atr->need_flush_ = 0;
        AudioTrackJni::AudioTrackFlushCatchAll(env, atr->audio_track_);
      }
      if (atr->need_set_volume_) {
        atr->need_set_volume_ = 0;
        AudioTrackJni::AudioTrackSetStereoVolumeCatchAll(
            env, atr->audio_track_, atr->left_volume_, atr->right_volume_);
      }
      if (atr->speed_changed_) {
        atr->speed_changed_ = 0;
        AudioTrackJni::AudioTracksetSpeedCatchAll(env, atr->audio_track_,
                                                  atr->speed_);
      }

      if (atr->dynamic_audiotrack_latency_ <= 0) {
        atr->dynamic_audiotrack_latency_ =
            static_cast<double>(AudioTrackJni::AudioTrackGetLatencyCatchAll(
                env, atr->audio_track_)) /
            1000.0;
      }
    }
    atr->audio_callback_->GetBuffer(atr->buffer_, copy_size);
    {
      std::unique_lock<std::mutex> mutex(atr->mutex_);
      if (atr->need_flush_) {
        atr->need_flush_ = 0;
        AudioTrackJni::AudioTrackFlushCatchAll(env, atr->audio_track_);
      }
    }

    if (!atr->byte_buffer_ || copy_size > atr->byte_buffer_capacity_) {
      if (atr->byte_buffer_) {
        JniDeleteGlobalRefP(env,
                            reinterpret_cast<jobject *>(&atr->byte_buffer_));
      }
      atr->byte_buffer_capacity_ = 0;
      int capacity = std::max(copy_size, atr->buffer_size_);
      atr->byte_buffer_ = JniNewByteArrayGlobalRefCatchAll(env, capacity);
      if (!atr->byte_buffer_) {
        AV_LOGE_ID(AUDIO_LOG_TAG, atr->GetSessionId(),
                   "[%s:%d] atr->byte_buffer_ is null!\n", __FUNCTION__,
                   __LINE__);
        return -1;
      }
      atr->byte_buffer_capacity_ = capacity;
    }
    env->SetByteArrayRegion(atr->byte_buffer_, 0, copy_size,
                            reinterpret_cast<jbyte *>(atr->buffer_));
    if (JniCheckExceptionCatchAll(env)) {
      AV_LOGE_ID(AUDIO_LOG_TAG, atr->GetSessionId(),
                 "[%s:%d] SetByteArrayRegion failed!\n", __FUNCTION__,
                 __LINE__);
      return -1;
    }
    int written = AudioTrackJni::AudioTrackWriteCatchAll(
        env, atr->audio_track_, atr->byte_buffer_, 0, copy_size);
    if (written != copy_size) {
      AV_LOGI_ID(AUDIO_LOG_TAG, atr->GetSessionId(),
                 "[%s:%d] written = %d,copy_size = %d!\n", __FUNCTION__,
                 __LINE__, written, copy_size);
    }
  }
  return 0;
}

void AudioTrackRender::PauseAudio(int pause_on) {
  std::unique_lock<std::mutex> mutex(mutex_);
  pause_on_ = pause_on;
  if (!pause_on_) {
    wakeup_cond_.notify_one();
  }
}

void AudioTrackRender::FlushAudio() {
  std::unique_lock<std::mutex> mutex(mutex_);
  need_flush_ = 1;
  wakeup_cond_.notify_one();
}

void AudioTrackRender::SetStereoVolume(float left_volume, float right_volume) {
  std::unique_lock<std::mutex> mutex(mutex_);
  left_volume_ = left_volume;
  right_volume_ = right_volume;
  need_set_volume_ = 1;
}

void AudioTrackRender::CloseAudio() {
  std::unique_lock<std::mutex> mutex(mutex_);
  abort_request_ = true;
  wakeup_cond_.notify_one();
}

void AudioTrackRender::WaitClose() {
  std::unique_lock<std::mutex> mutex(thread_mutex_);
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] AudioTrackRender start\n",
             __FUNCTION__, __LINE__);
  abort_request_ = true;
  if (aout_thread_ && aout_thread_->joinable()) {
    aout_thread_->join();
    delete aout_thread_;
    aout_thread_ = nullptr;
  }
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] AudioTrackRender end\n",
             __FUNCTION__, __LINE__);
}

double AudioTrackRender::GetLatencySeconds() {
  return minimal_latency_seconds_;
}

double AudioTrackRender::GetAudiotrackLatencySeconds() {
  return dynamic_audiotrack_latency_;
}

void AudioTrackRender::SetDefaultLatencySeconds(double latency) {
  minimal_latency_seconds_ = latency;
}

int AudioTrackRender::GetAudioPerSecondCallBacks() {
  return KAudioMaxCallbacksPerSec;
}

// optional
void AudioTrackRender::SetPlaybackRate(float playbackRate) {
  std::unique_lock<std::mutex> mutex(mutex_);
  speed_ = playbackRate;
  speed_changed_ = 1;
  wakeup_cond_.notify_one();
}
void AudioTrackRender::SetPlaybackVolume(float volume) { return; }

// android only
int AudioTrackRender::GetAudioSessionId() { return audio_session_id_; }
#endif
