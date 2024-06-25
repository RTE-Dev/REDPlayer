#pragma once

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaFormat.h"
#include "reddecoder/video/video_codec_info.h"
#include "reddecoder/video/video_common_definition.h"
#include "reddecoder/video/video_decoder/video_decoder.h"

namespace reddecoder {

struct AMediaFormatReleaser {
  void operator()(AMediaFormat *ptr) const { AMediaFormat_delete(ptr); }
};

struct AMediaCodecReleaser {
  void operator()(AMediaCodec *ptr) const { AMediaCodec_delete(ptr); }
};

class Ndk_MediaCodecVideoDecoder : public VideoDecoder {
public:
  Ndk_MediaCodecVideoDecoder(VideoCodecInfo codec);
  ~Ndk_MediaCodecVideoDecoder() override;
  VideoCodecError init(const Buffer *buffer) override;
  VideoCodecError release() override;
  VideoCodecError decode(const Buffer *buffer) override;
  VideoCodecError flush() override;
  VideoCodecError set_video_format_description(const Buffer *buffer) override;
  VideoCodecError get_delayed_frames() override;
  VideoCodecError get_delayed_frame() override;
  VideoCodecError set_hardware_context(HardWareContext *context) override;
  VideoCodecError update_hardware_context(HardWareContext *context) override;

  int get_serial();

private:
  VideoCodecError init_media_codec();
  VideoCodecError release_media_codec();

  VideoCodecError init_media_format_desc(const Buffer *buffer);
  VideoCodecError release_media_format_desc();

  VideoCodecError feed_decoder(const Buffer *buffer, size_t &offset);
  VideoCodecError drain_decoder();

  std::unique_ptr<AMediaFormat, AMediaFormatReleaser> media_format_;
  std::unique_ptr<AMediaCodec, AMediaCodecReleaser> media_codec_;
  ANativeWindow *native_window_{nullptr};
  std::atomic_int serial_;
  bool is_eof_state_ = false;
  int current_input_buffer_ = -1;

  CodecContext codec_ctx_;

  bool is_drain_state_ = false;
  bool output_first_frame_state_ = false;
  bool is_mediacodec_start_ = false;
};

} // namespace reddecoder
