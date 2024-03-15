#pragma once

#include <memory>

#include "reddecoder/audio/audio_codec_info.h"
#include "reddecoder/audio/audio_common_definition.h"
#include "reddecoder/common/buffer.h"

namespace reddecoder {

class AudioDecodedCallback {
 public:
  virtual AudioCodecError on_decoded_frame(
      std::unique_ptr<Buffer> decoded_frame) = 0;
  virtual void on_decode_error(AudioCodecError error) = 0;
  virtual ~AudioDecodedCallback() = default;
};

class AudioDecoder {
 public:
  AudioDecoder(AudioCodecInfo codec);
  virtual ~AudioDecoder() = default;
  virtual AudioCodecError init(AudioCodecConfig& config) = 0;
  virtual AudioCodecError release() = 0;
  virtual AudioCodecError decode(const Buffer* buffer) = 0;
  virtual AudioCodecError register_decode_complete_callback(
      AudioDecodedCallback* callback);

 protected:
  AudioDecodedCallback* audio_decoded_callback_ = nullptr;
  AudioCodecInfo codec_info_;
};
}  // namespace reddecoder