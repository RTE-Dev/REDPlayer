#pragma once
#include "./audio_info.h"
#include <memory>

NS_REDRENDER_AUDIO_BEGIN
class IAudioRender {
public:
  explicit IAudioRender(const int &session_id) : session_id_(session_id) {}
  int GetSessionId() { return session_id_; }
  virtual ~IAudioRender() = default;
  virtual int OpenAudio(const AudioInfo &desired, AudioInfo &obtained,
                        std::unique_ptr<AudioCallback> &audio_callback) = 0;
  virtual void PauseAudio(int pause_on) = 0;
  virtual void FlushAudio() = 0;
  virtual void SetStereoVolume(float left_volume, float right_volume) = 0;
  virtual void CloseAudio() = 0;
  virtual void WaitClose() = 0;

  virtual double GetLatencySeconds() = 0;
  virtual double GetAudiotrackLatencySeconds() = 0;
  virtual void SetDefaultLatencySeconds(double latency) = 0;
  virtual int GetAudioPerSecondCallBacks() = 0;

  // optional
  virtual void SetPlaybackRate(float playbackRate) = 0;
  virtual void SetPlaybackVolume(float volume) = 0;

  // android only
  virtual int GetAudioSessionId() = 0;

protected:
  const int session_id_{0};
  std::unique_ptr<AudioCallback> audio_callback_{nullptr};
};
NS_REDRENDER_AUDIO_END
