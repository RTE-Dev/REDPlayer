#if defined(__APPLE__)
#pragma once
#include <AudioToolbox/AudioToolbox.h>

#include <mutex>

#include "../audio_render_interface.h"
NS_REDRENDER_AUDIO_BEGIN

class AudioQueueRender : public IAudioRender {
public:
  AudioQueueRender();
  explicit AudioQueueRender(const int &session_id);
  ~AudioQueueRender();
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
  static void AudioQueueOuptutCallback(void *user_data, AudioQueueRef aq,
                                       AudioQueueBufferRef buffer);

private:
  AudioQueueBufferRef audio_queue_buffer_ref_array_[KAudioQueueNumberBuffers]{};
  AudioQueueRef audio_queue_ref_{nullptr};
  std::mutex mutex_;
  AudioInfo obtained_{};
  bool is_paused_{false};
  bool is_stopped_{false};
  volatile bool is_aborted_{false};
};
NS_REDRENDER_AUDIO_END
#endif
