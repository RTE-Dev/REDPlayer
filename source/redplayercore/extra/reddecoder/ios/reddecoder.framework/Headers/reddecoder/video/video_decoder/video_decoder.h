#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "reddecoder/common/buffer.h"
#include "reddecoder/video/video_codec_info.h"
#include "reddecoder/video/video_common_definition.h"

namespace reddecoder {

struct CodecContext {
  int width = 0;
  int height = 0;
  bool is_hdr = false;
  bool is_annexb_extradata = false;
  int nal_size = 4;
  int rotate_degree = 0;
  VideoColorRange color_range = VideoColorRange::kMPEG;
  VideoSampleAspectRatio sample_aspect_ratio;
};

class VideoDecodedCallback {
 public:
  virtual VideoCodecError on_decoded_frame(
      std::unique_ptr<Buffer> decoded_frame) = 0;
  virtual void on_decode_error(VideoCodecError error,
                               int internal_error_code = 0) = 0;
  virtual ~VideoDecodedCallback() = default;
};

class VideoDecoder {
 public:
  VideoDecoder(VideoCodecInfo codec);
  virtual ~VideoDecoder();
  virtual VideoCodecError init(const Buffer* buffer = nullptr) = 0;
  virtual VideoCodecError release() = 0;
  virtual VideoCodecError decode(const Buffer* buffer) = 0;
  virtual VideoCodecError register_decode_complete_callback(
      VideoDecodedCallback* callback, bool free_callback_when_release = false);
  virtual VideoDecodedCallback* get_decode_complete_callback();
  virtual VideoCodecError set_option(std::string key, std::string value);
  virtual std::string get_option(std::string key);

  /* for now only support extradata type description */
  virtual VideoCodecError set_video_format_description(
      const Buffer* buffer) = 0;

  /* flush internal buffer for reuse, flush will retain video format info
  call set_video_format_description to reconfig */
  virtual VideoCodecError flush() = 0;

  /* get delayed frames in decoder buffer */
  virtual VideoCodecError get_delayed_frames() = 0;
  virtual VideoCodecError get_delayed_frame() = 0;
  virtual VideoCodecError set_hardware_context(HardWareContext* context) {
    return VideoCodecError::kNotSupported;
  }
  virtual VideoCodecError update_hardware_context(HardWareContext* context) {
    return VideoCodecError::kNotSupported;
  }
  virtual VideoCodecError set_skip_frame(int skip_frame) {
    return VideoCodecError::kNotSupported;
  }

 protected:
  VideoDecodedCallback* video_decoded_callback_ = nullptr;
  bool free_callback_when_release_ = false;
  VideoCodecInfo codec_info_;
  std::map<std::string, std::string> options_;
  std::mutex option_mutex_;
};
}  // namespace reddecoder