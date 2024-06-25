#include "reddecoder/video/video_decoder/video_decoder_factory.h"
#include "reddecoder/common/logger.h"
#include "reddecoder/video/video_decoder/ffmpeg_video_decoder.h"

#if defined(__APPLE__)
#include "reddecoder/video/video_decoder/ios/vtb_video_decoder.h"
#endif

#if defined(__ANDROID__)
#include "reddecoder/video/video_decoder/android/ndk_mediacodec_video_decoder.h"
#endif

#ifdef __HARMONY__
#include "reddecoder/video/video_decoder/harmony/harmony_video_decoder.h"
#endif

namespace reddecoder {
VideoDecoderFactory::VideoDecoderFactory() {
  supported_codecs_ = {
      VideoCodecInfo(VideoCodecName::kH264,
                     VideoCodecImplementationType::kSoftware),
      VideoCodecInfo(VideoCodecName::kH265,
                     VideoCodecImplementationType::kSoftware),
      VideoCodecInfo(VideoCodecName::kFFMPEGID,
                     VideoCodecImplementationType::kSoftware),
      VideoCodecInfo(VideoCodecName::kH265,
                     VideoCodecImplementationType::kHardwareHarmony),
      VideoCodecInfo(VideoCodecName::kH264,
                     VideoCodecImplementationType::kHardwareHarmony),
      VideoCodecInfo(VideoCodecName::kH264,
                     VideoCodecImplementationType::kHardwareAndroid),
      VideoCodecInfo(VideoCodecName::kH265,
                     VideoCodecImplementationType::kHardwareAndroid),
      VideoCodecInfo(VideoCodecName::kH264,
                     VideoCodecImplementationType::kHardwareiOS),
      VideoCodecInfo(VideoCodecName::kH265,
                     VideoCodecImplementationType::kHardwareiOS),
  };
}

VideoDecoderFactory::~VideoDecoderFactory() {}

std::unique_ptr<VideoDecoder>
VideoDecoderFactory::create_video_decoder(const VideoCodecInfo &codec) {
  std::unique_ptr<VideoDecoder> decoder;
  if (!is_codec_supported(codec)) {
    AV_LOGE(DEC_TAG, "[VideoDecoderFactory] %s, codec not supported\n",
            __FUNCTION__);
    return decoder;
  }

  switch (codec.implementation_type) {
  case VideoCodecImplementationType::kSoftware:
    decoder = std::make_unique<FFmpegVideoDecoder>(codec);
    break;
#if defined(__ANDROID__)
  case VideoCodecImplementationType::kHardwareAndroid:
    decoder = std::make_unique<Ndk_MediaCodecVideoDecoder>(codec);
    break;
#endif
#ifdef __HARMONY__
  case VideoCodecImplementationType::kHardwareHarmony:
    decoder = std::make_unique<HarmonyVideoDecoder>(codec);
    break;
#endif
#if defined(__APPLE__)
  case VideoCodecImplementationType::kHardwareiOS:
    decoder = std::make_unique<VtbVideoDecoder>(codec);
    break;
#endif
  default:
    break;
  }
  return decoder;
}

std::vector<VideoCodecInfo> VideoDecoderFactory::get_supported_codecs() {
  return supported_codecs_;
}

bool VideoDecoderFactory::is_codec_supported(const VideoCodecInfo &codec) {
  for (auto tmp_codec : supported_codecs_) {
    if (tmp_codec.codec_name == codec.codec_name &&
        tmp_codec.implementation_type == codec.implementation_type) {
      return true;
    }
  }
  return false;
}

} // namespace reddecoder
