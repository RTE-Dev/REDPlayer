#pragma once

namespace reddecoder {

enum class VideoCodecName {
  kUnknown = 0,
  kH264 = 1,
  kH265 = 2,
  kAV1 = 3,
  kFFMPEGID = 4,
};

enum class VideoCodecImplementationType {
  kUnknown = 0,
  kSoftware = 1,
  kHardwareAndroid = 2,
  kHardwareiOS = 3,
  kHardwareHarmony = 4
};

enum class VideoCodecProfile {
  kUnknown = 0,
  kMain = 1,
};

enum class VideoCodecLevel {
  kUnknown = 0,
};

struct VideoCodecInfo {
  VideoCodecInfo(VideoCodecName name, VideoCodecImplementationType type);
  ~VideoCodecInfo() = default;
  VideoCodecName codec_name;
  VideoCodecProfile profile;
  VideoCodecLevel level;
  VideoCodecImplementationType implementation_type;
  int ffmpeg_codec_id;
};
} // namespace reddecoder
