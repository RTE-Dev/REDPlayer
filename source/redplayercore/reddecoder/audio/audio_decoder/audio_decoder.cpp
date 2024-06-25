#include "reddecoder/audio/audio_decoder/audio_decoder.h"

namespace reddecoder {

AudioDecoder::AudioDecoder(AudioCodecInfo codec) : codec_info_(codec) {}

AudioCodecError AudioDecoder::register_decode_complete_callback(
    AudioDecodedCallback *callback) {
  // log
  audio_decoded_callback_ = callback;
  return AudioCodecError::kNoError;
}

} // namespace reddecoder
