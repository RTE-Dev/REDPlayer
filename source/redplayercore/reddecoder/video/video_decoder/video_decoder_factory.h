#pragma once

#include <memory>
#include <vector>

#include "reddecoder/video/video_codec_info.h"
#include "reddecoder/video/video_decoder/video_decoder.h"

namespace reddecoder {
class VideoDecoderFactory {
public:
  VideoDecoderFactory();
  ~VideoDecoderFactory();
  std::unique_ptr<VideoDecoder>
  create_video_decoder(const VideoCodecInfo &codec);
  std::vector<VideoCodecInfo> get_supported_codecs();
  bool is_codec_supported(const VideoCodecInfo &codec);

private:
  std::vector<VideoCodecInfo> supported_codecs_;
};
} // namespace reddecoder
