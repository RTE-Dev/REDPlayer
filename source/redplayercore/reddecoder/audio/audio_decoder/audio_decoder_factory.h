#pragma once

#include <memory>
#include <vector>

#include "reddecoder/audio/audio_codec_info.h"
#include "reddecoder/audio/audio_decoder/audio_decoder.h"

namespace reddecoder {
class AudioDecoderFactory {
public:
  AudioDecoderFactory();
  ~AudioDecoderFactory();
  std::unique_ptr<AudioDecoder>
  create_audio_decoder(const AudioCodecInfo &codec);
  std::vector<AudioCodecInfo> get_supported_codecs();
  bool is_codec_supported(const AudioCodecInfo &codec);

private:
  std::vector<AudioCodecInfo> supported_codecs_;
};
} // namespace reddecoder
