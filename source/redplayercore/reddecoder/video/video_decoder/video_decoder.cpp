#include "reddecoder/video/video_decoder/video_decoder.h"

#include "reddecoder/common/logger.h"

namespace reddecoder {

VideoDecoder::VideoDecoder(VideoCodecInfo codec) : codec_info_(codec) {}
VideoDecoder::~VideoDecoder() {
  if (free_callback_when_release_) {
    delete video_decoded_callback_;
  }
  options_.clear();
}

VideoCodecError VideoDecoder::set_option(std::string key, std::string value) {
  std::lock_guard<std::mutex> lock(option_mutex_);
  auto iter = options_.find(key);
  if (iter != options_.end()) {
    options_.erase(iter);
  }
  options_.emplace(key, value);
  return VideoCodecError::kNoError;
}

std::string VideoDecoder::get_option(std::string key) {
  std::lock_guard<std::mutex> lock(option_mutex_);
  auto iter = options_.find(key);
  if (iter != options_.end()) {
    return iter->second;
  }
  return "";
}

VideoCodecError VideoDecoder::register_decode_complete_callback(
    VideoDecodedCallback *callback, bool free_callback_when_release) {
  AV_LOGI(DEC_TAG, "[reddecoder] %s, register video decode callback\n",
          __FUNCTION__);
  video_decoded_callback_ = callback;
  free_callback_when_release_ = free_callback_when_release;
  return VideoCodecError::kNoError;
}

VideoDecodedCallback *VideoDecoder::get_decode_complete_callback() {
  return video_decoded_callback_;
}

} // namespace reddecoder
