//
// Created by zhangqingyu on 2023/10/27.
//

#if defined(__HARMONY__)

#include "./harmony_audio_render.h"
#include "../audio_common.h"

USING_NS_REDRENDER_AUDIO

int32_t HarmonyAudioRender::HarmonyAudioRendererOnWriteDataCallback(
    OH_AudioRenderer *renderer, void *user_data, void *buffer, int32_t length) {
  HarmonyAudioRender *audio_renderer =
      reinterpret_cast<HarmonyAudioRender *>(user_data);
  if (!audio_renderer || !audio_renderer->audio_callback_) {
    return 0;
  }

  audio_renderer->audio_callback_->GetBuffer(static_cast<uint8_t *>(buffer),
                                             length);
  return length;
}

int32_t HarmonyAudioRender::HarmonyAudioRendererOnStreamEventCallback(
    OH_AudioRenderer *renderer, void *user_data, OH_AudioStream_Event event) {
  HarmonyAudioRender *audio_renderer =
      reinterpret_cast<HarmonyAudioRender *>(user_data);
  if (!audio_renderer) {
    return 0;
  }
  AV_LOGI_ID(AUDIO_LOG_TAG, audio_renderer->GetSessionId(),
             "[%s:%d] event is %d\n", __FUNCTION__, __LINE__, event);
  return 0;
}

int32_t HarmonyAudioRender::HarmonyAudioRendererOnInterruptEvenCallback(
    OH_AudioRenderer *renderer, void *user_data,
    OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint) {
  HarmonyAudioRender *audio_renderer =
      reinterpret_cast<HarmonyAudioRender *>(user_data);
  if (!audio_renderer) {
    return 0;
  }
  AV_LOGI_ID(AUDIO_LOG_TAG, audio_renderer->GetSessionId(),
             "[%s:%d] type:%d, hint:%d\n", __FUNCTION__, __LINE__, type, hint);
  return 0;
}

int32_t HarmonyAudioRender::HarmonyAudioRendererOnErrorCallback(
    OH_AudioRenderer *renderer, void *user_data, OH_AudioStream_Result error) {
  HarmonyAudioRender *audio_renderer =
      reinterpret_cast<HarmonyAudioRender *>(user_data);
  if (!audio_renderer) {
    return 0;
  }
  AV_LOGI_ID(AUDIO_LOG_TAG, audio_renderer->GetSessionId(),
             "[%s:%d] error:%d\n", __FUNCTION__, __LINE__, error);
  return 0;
}

HarmonyAudioRender::HarmonyAudioRender() : HarmonyAudioRender(0) {}

HarmonyAudioRender::HarmonyAudioRender(const int &session_id)
    : IAudioRender(session_id) {}

HarmonyAudioRender::~HarmonyAudioRender() {
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_,
             "[%s:%d] HarmonyAudioRender Deconstruct.\n", __FUNCTION__,
             __LINE__);
}

void HarmonyAudioRender::AdjustAudioInfo(const AudioInfo &desired,
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

int HarmonyAudioRender::OpenAudio(
    const AudioInfo &desired, AudioInfo &obtained,
    std::unique_ptr<AudioCallback> &audio_callback) {
  AdjustAudioInfo(desired, obtained);
  int ret = 0;
  uint32_t stream_id = 0;
  int32_t frame_size = 0;
  OH_AudioStream_Result audio_stream_result =
      OH_AudioStream_Result::AUDIOSTREAM_SUCCESS;
  OH_AudioStreamBuilder *stream_build = nullptr;
  OH_AudioRenderer *audioRenderer = nullptr;
  OH_AudioRenderer_Callbacks callbacks;

  audio_stream_result = OH_AudioStreamBuilder_Create(
      &stream_build, OH_AudioStream_Type::AUDIOSTREAM_TYPE_RENDERER);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] OH_AudioStreamBuilder_Create result %d, failed!\n",
               __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  audio_stream_result =
      OH_AudioStreamBuilder_SetSamplingRate(stream_build, obtained.sample_rate);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_SetSamplingRate result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  audio_stream_result =
      OH_AudioStreamBuilder_SetChannelCount(stream_build, obtained.channels);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_SetChannelCount result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  audio_stream_result = OH_AudioStreamBuilder_SetSampleFormat(
      stream_build, OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S16LE);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_SetSampleFormat result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  audio_stream_result = OH_AudioStreamBuilder_SetEncodingType(
      stream_build, OH_AudioStream_EncodingType::AUDIOSTREAM_ENCODING_TYPE_RAW);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_SetSampleFormat result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  audio_stream_result = OH_AudioStreamBuilder_SetLatencyMode(
      stream_build, OH_AudioStream_LatencyMode::AUDIOSTREAM_LATENCY_MODE_FAST);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_SetLatencyMode result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  audio_stream_result = OH_AudioStreamBuilder_SetRendererInfo(
      stream_build, OH_AudioStream_Usage::AUDIOSTREAM_USAGE_MOVIE);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_SetRendererInfo result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }

  callbacks.OH_AudioRenderer_OnWriteData =
      HarmonyAudioRendererOnWriteDataCallback;
  callbacks.OH_AudioRenderer_OnStreamEvent =
      HarmonyAudioRendererOnStreamEventCallback;
  callbacks.OH_AudioRenderer_OnInterruptEvent =
      HarmonyAudioRendererOnInterruptEvenCallback;
  callbacks.OH_AudioRenderer_OnError = HarmonyAudioRendererOnErrorCallback;

  audio_stream_result =
      OH_AudioStreamBuilder_SetRendererCallback(stream_build, callbacks, this);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] OH_AudioStreamBuilder_SetRendererCallback result %d, "
               "failed!\n",
               __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }
  audio_stream_result =
      OH_AudioStreamBuilder_GenerateRenderer(stream_build, &audioRenderer);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioStreamBuilder_GenerateRenderer result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }
  OH_AudioRenderer_GetStreamId(audioRenderer, &stream_id);

  audio_stream_result =
      OH_AudioRenderer_GetFrameSizeInCallback(audioRenderer, &frame_size);
  if (audio_stream_result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
    AV_LOGE_ID(
        AUDIO_LOG_TAG, session_id_,
        "[%s:%d] OH_AudioRenderer_GetFrameSizeInCallback result %d, failed!\n",
        __FUNCTION__, __LINE__, audio_stream_result);
    ret = -1;
    goto end;
  }
  obtained.size = frame_size;

end:
  if (stream_build != nullptr) {
    OH_AudioStreamBuilder_Destroy(stream_build);
  }
  std::unique_lock<std::mutex> mutex(mutex_);
  audio_render_ = audioRenderer;
  audio_session_id_ = stream_id;
  audio_callback_ = std::move(audio_callback);
  return ret;
}

void HarmonyAudioRender::PauseAudio(int pause_on) {
  std::unique_lock<std::mutex> mutex(mutex_);
  if (audio_render_ != nullptr) {
    if (pause_on) {
      OH_AudioStream_Result result = OH_AudioRenderer_Pause(audio_render_);
      if (result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
        AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
                   "[%s:%d] OH_AudioRenderer_Pause result %d, failed!\n",
                   __FUNCTION__, __LINE__, result);
      }
    } else {
      OH_AudioStream_Result result = OH_AudioRenderer_Start(audio_render_);
      if (result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
        AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
                   "[%s:%d] OH_AudioRenderer_Start result %d, failed!\n",
                   __FUNCTION__, __LINE__, result);
      }
    }
  }
}

void HarmonyAudioRender::FlushAudio() {
  std::unique_lock<std::mutex> mutex(mutex_);
  if (audio_render_ != nullptr) {
    OH_AudioStream_Result result = OH_AudioRenderer_Flush(audio_render_);
    if (result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
                 "[%s:%d] OH_AudioRenderer_Flush result %d, failed!\n",
                 __FUNCTION__, __LINE__, result);
    }
  }
}

void HarmonyAudioRender::SetStereoVolume(float left_volume,
                                         float right_volume) {
  AV_LOGD_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] end\n", __FUNCTION__,
             __LINE__);
}

void HarmonyAudioRender::CloseAudio() {
  std::unique_lock<std::mutex> mutex(mutex_);
  if (audio_render_ != nullptr) {
    OH_AudioStream_Result result = OH_AudioRenderer_Release(audio_render_);
    if (result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
                 "[%s:%d] OH_AudioRenderer_Release result %d, failed!\n",
                 __FUNCTION__, __LINE__, result);
    }
    audio_render_ = nullptr;
  }
}

void HarmonyAudioRender::WaitClose() {}

double HarmonyAudioRender::GetLatencySeconds() {
  return minimal_latency_seconds_;
}

double HarmonyAudioRender::GetAudiotrackLatencySeconds() { return 0.f; }

void HarmonyAudioRender::SetDefaultLatencySeconds(double latency) {
  minimal_latency_seconds_ = latency;
}

int HarmonyAudioRender::GetAudioPerSecondCallBacks() {
  return KAudioMaxCallbacksPerSec;
}

// optional
void HarmonyAudioRender::SetPlaybackRate(float playbackRate) {
  std::unique_lock<std::mutex> mutex(mutex_);
  if (audio_render_ != nullptr) {
    OH_AudioStream_Result result =
        OH_AudioRenderer_SetSpeed(audio_render_, playbackRate);
    if (result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
                 "[%s:%d] OH_AudioRenderer_SetSpeed result %d, failed!\n",
                 __FUNCTION__, __LINE__, result);
    }
  }
}

void HarmonyAudioRender::SetPlaybackVolume(float volume) {
  std::unique_lock<std::mutex> mutex(mutex_);
  if (audio_render_ != nullptr) {
    OH_AudioStream_Result result =
        OH_AudioRenderer_SetVolume(audio_render_, volume);
    if (result != OH_AudioStream_Result::AUDIOSTREAM_SUCCESS) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
                 "[%s:%d] OH_AudioRenderer_SetVolume result %d, failed!\n",
                 __FUNCTION__, __LINE__, result);
    }
  }
}

int HarmonyAudioRender::GetAudioSessionId() { return audio_session_id_; }
#endif
