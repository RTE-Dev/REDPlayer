#pragma once
#include <cstdint>

#include "../redrender_macro_definition.h"

NS_REDRENDER_AUDIO_BEGIN
constexpr int KAudioMaxCallbacksPerSec = 30;
constexpr int KAudioQueueNumberBuffers = 3;
constexpr int KAudioMinBuferSize = 512;

enum class AudioFormat {
  kAudioInvalid = 0,
  kAudioS16Sys = 1,
  kAudioS32Sys = 2,
  kAudioF32Sys = 3,
  kAudioU8 = 4,
};

NS_REDRENDER_AUDIO_END
