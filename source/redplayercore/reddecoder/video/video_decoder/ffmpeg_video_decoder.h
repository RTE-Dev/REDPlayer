#pragma once

#include "reddecoder/video/video_codec_info.h"
#include "reddecoder/video/video_decoder/video_decoder.h"

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

class FFmpegVideoDecoder : public VideoDecoder {
public:
  FFmpegVideoDecoder(VideoCodecInfo codec);
  ~FFmpegVideoDecoder() override;
  VideoCodecError init(const Buffer *buffer) override;
  VideoCodecError release() override;
  VideoCodecError decode(const Buffer *buffer) override;
  VideoCodecError flush() override;
  VideoCodecError set_video_format_description(const Buffer *buffer) override;
  VideoCodecError get_delayed_frames() override;
  VideoCodecError get_delayed_frame() override;
  VideoCodecError set_skip_frame(int skip_frame) override;

  AVDiscard transfer_discard_opt_to_ffmpeg(VideoDiscard opt);
  std::unique_ptr<AVCodecContext, AVCodecContextReleaser> codec_context_;
  bool is_drain_state_ = false;
};

} // namespace reddecoder
