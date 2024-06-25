//
// Created by zhangqingyu on 2023/10/27.
//

#if defined(__HARMONY__)
#pragma once

#include <condition_variable>
#include <iostream>
#include <thread>

#include "ohaudio/native_audiorenderer.h"
#include "ohaudio/native_audiostreambuilder.h"

#include "../audio_render_interface.h"

NS_REDRENDER_AUDIO_BEGIN

class HarmonyAudioRender : public IAudioRender {
public:
  HarmonyAudioRender();
  explicit HarmonyAudioRender(const int &session_id);
  ~HarmonyAudioRender();

  int OpenAudio(const AudioInfo &desired, AudioInfo &obtained,
                std::unique_ptr<AudioCallback> &audio_callback) override;
  void PauseAudio(int pause_on) override;
  void FlushAudio() override;
  void SetStereoVolume(float left_volume, float right_volume) override;
  void CloseAudio() override;
  void WaitClose() override;

  double GetLatencySeconds() override;
  double GetAudiotrackLatencySeconds() override;
  void SetDefaultLatencySeconds(double latency) override;
  int GetAudioPerSecondCallBacks() override;

  // optional
  void SetPlaybackRate(float playbackRate) override;
  void SetPlaybackVolume(float volume) override;

  // android only
  int GetAudioSessionId() override;

private:
  void AdjustAudioInfo(const AudioInfo &desired, AudioInfo &obtained);

  static int32_t HarmonyAudioRendererOnWriteDataCallback(
      OH_AudioRenderer *renderer, void *user_data, void *buffer, int32_t lenth);
  static int32_t HarmonyAudioRendererOnStreamEventCallback(
      OH_AudioRenderer *renderer, void *user_data, OH_AudioStream_Event event);
  static int32_t HarmonyAudioRendererOnInterruptEvenCallback(
      OH_AudioRenderer *renderer, void *user_data,
      OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint);
  static int32_t HarmonyAudioRendererOnErrorCallback(
      OH_AudioRenderer *renderer, void *user_data, OH_AudioStream_Result error);

private:
  OH_AudioRenderer *audio_render_{nullptr};
  int audio_session_id_{-1};
  double minimal_latency_seconds_{0};
  std::mutex mutex_;
};
NS_REDRENDER_AUDIO_END

#endif
