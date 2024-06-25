#if defined(__ANDROID__)
#pragma once
#include <jni.h>

#include <condition_variable>
#include <iostream>
#include <thread>

#include "../audio_render_interface.h"
#include "jni/audio_track_jni.h"

NS_REDRENDER_AUDIO_BEGIN
class AudioTrackRender : public IAudioRender {
public:
  AudioTrackRender();
  explicit AudioTrackRender(const int &session_id);
  ~AudioTrackRender();
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
  static int audio_loop(AudioTrackRender *atr);

private:
  jobject audio_track_{nullptr};
  std::thread *aout_thread_{nullptr};
  std::mutex mutex_;
  std::mutex thread_mutex_;
  std::condition_variable wakeup_cond_;
  jbyteArray byte_buffer_{nullptr};
  int byte_buffer_capacity_{0};
  uint8_t *buffer_{nullptr};
  int buffer_size_{0};
  volatile bool need_flush_{false};
  volatile bool pause_on_{true};
  volatile bool abort_request_{false};
  volatile bool need_set_volume_{false};
  volatile float left_volume_{0};
  volatile float right_volume_{0};
  int audio_session_id_{-1};
  volatile float speed_{0};
  volatile bool speed_changed_{0};
  double minimal_latency_seconds_{0};
  double dynamic_audiotrack_latency_{0};
};
NS_REDRENDER_AUDIO_END

#endif
