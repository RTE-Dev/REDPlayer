#pragma once

#include "reddecoder/audio/audio_codec_info.h"
#include "reddecoder/audio/audio_decoder/audio_decoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace reddecoder {

struct AVCodecContextReleaser {
  void operator()(AVCodecContext *ptr) const { avcodec_free_context(&ptr); }
};

struct AVFrameReleaser {
  void operator()(AVFrame *ptr) const { av_frame_free(&ptr); }
};

class FFmpegAudioDecoder : public AudioDecoder {
public:
  FFmpegAudioDecoder(AudioCodecInfo codec);
  ~FFmpegAudioDecoder() override;
  AudioCodecError init(AudioCodecConfig &config) override;
  AudioCodecError release() override;
  AudioCodecError decode(const Buffer *buffer) override;
  AudioCodecError flush() override;

  std::unique_ptr<AVCodecContext, AVCodecContextReleaser> codec_context_;
};

} // namespace reddecoder
