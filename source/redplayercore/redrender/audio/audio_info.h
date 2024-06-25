#pragma once
#include <cstdint>
#include <iostream>

#include "../redrender_macro_definition.h"
#include "./audio_common.h"

NS_REDRENDER_AUDIO_BEGIN
struct AudioInfo {
  int channels;
  int sample_rate;
  uint64_t channel_layout;
  // for audioqueue
  uint8_t silence;
  uint16_t samples;
  uint32_t size;
  AudioFormat format;
  AudioInfo &operator=(const AudioInfo &info) {
    this->sample_rate = info.sample_rate;
    this->channels = info.channels;
    this->format = info.format;
    this->channel_layout = info.channel_layout;
    this->silence = info.silence;
    this->samples = info.samples;
    this->size = info.size;
    this->format = info.format;
    return *this;
  }
};

class AudioCallback {
public:
  explicit AudioCallback(void *user_data) : user_data_(user_data) {}
  virtual ~AudioCallback() = default;
  virtual void GetBuffer(uint8_t *stream, int len) { return; }

protected:
  void *user_data_{nullptr};
};

NS_REDRENDER_AUDIO_END
