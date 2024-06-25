#include "reddecoder/audio/audio_decoder/audio_decoder_factory.h"

#include "reddecoder/audio/audio_decoder/ffmpeg_audio_decoder.h"

namespace reddecoder {
AudioDecoderFactory::AudioDecoderFactory() {
  supported_codecs_ = {
      AudioCodecInfo(AudioCodecName::kAAC, AudioCodecProfile::kLC,
                     AudioCodecImplementationType::kSoftware),
      AudioCodecInfo(AudioCodecName::kAAC, AudioCodecProfile::kHev1,
                     AudioCodecImplementationType::kSoftware),
      AudioCodecInfo(AudioCodecName::kAAC, AudioCodecProfile::kHev2,
                     AudioCodecImplementationType::kSoftware),
      AudioCodecInfo(AudioCodecName::kOpaque, AudioCodecProfile::kUnknown,
                     AudioCodecImplementationType::kSoftware),
  };
}

AudioDecoderFactory::~AudioDecoderFactory() {}

std::unique_ptr<AudioDecoder>
AudioDecoderFactory::create_audio_decoder(const AudioCodecInfo &codec) {
  std::unique_ptr<AudioDecoder> decoder;
  if (!is_codec_supported(codec)) {
    // log
    return decoder;
  }

  // for now, we only have ffmpeg audio decoder
  decoder = std::make_unique<FFmpegAudioDecoder>(codec);

  return decoder;
}

std::vector<AudioCodecInfo> AudioDecoderFactory::get_supported_codecs() {
  return supported_codecs_;
}

bool AudioDecoderFactory::is_codec_supported(const AudioCodecInfo &codec) {
  for (auto tmp_codec : supported_codecs_) {
    if ((tmp_codec.codec_name == codec.codec_name &&
         tmp_codec.profile == codec.profile &&
         tmp_codec.implementation_type == codec.implementation_type)) {
      return true;
    }
  }
  return false;
}

} // namespace reddecoder
