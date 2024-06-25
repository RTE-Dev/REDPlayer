#pragma once

#include <cstddef>
#include <cstdint>
namespace reddecoder {

enum class AudioCodecName {
  kUnknown = 0,
  kAAC = 1,
  kOpaque = 2,
};

enum class AudioCodecImplementationType {
  kUnknown = 0,
  kSoftware = 1,
  kHardwareAndroid = 2,
  kHardwareiOS = 3,
};

enum class AudioCodecProfile {
  kUnknown = 0,
  kMain = 1,
  kHev1 = 2,
  kHev2 = 3,
  kLC = 4,
};

struct AudioCodecConfig {
  int sample_rate = 0;
  int channels = 0;
  uint8_t *extradata = nullptr;
  int extradata_size = 0;
};

struct AudioCodecInfo {
  AudioCodecInfo(AudioCodecName name, AudioCodecProfile profile,
                 AudioCodecImplementationType type);
  ~AudioCodecInfo() = default;
  AudioCodecName codec_name;
  AudioCodecProfile profile;
  AudioCodecImplementationType implementation_type;
  int opaque_codec_id;
  int opaque_profile;
};
} // namespace reddecoder
