#ifdef __HARMONY__
#include <sys/types.h>
#endif

#include "reddecoder/common/buffer.h"
#include <algorithm>

namespace reddecoder {
Buffer::Buffer(BufferType type, uint8_t *data, size_t size, bool own_data)
    : type_(type), data_(data), size_(size), capacity_(size),
      own_data_(own_data) {
  init_type(type);
}

Buffer::Buffer(BufferType type, size_t capacity)
    : type_(type), data_(new uint8_t[capacity]), size_(0), capacity_(capacity),
      own_data_(true) {
  init_type(type);
}

void Buffer::replace_buffer(uint8_t *data, size_t size, bool own_data) {
  if (own_data_) {
    delete[] data_;
  }

  data_ = data;
  size_ = size;
  own_data_ = own_data;
}

void Buffer::init_type(BufferType type) {
  switch (type) {
  case BufferType::kVideoFrame:
    video_frame_meta_ = std::make_unique<VideoFrameMeta>();
    break;
  case BufferType::kAudioFrame:
    audio_frame_meta_ = std::make_unique<AudioFrameMeta>();
    break;
  case BufferType::kVideoFormatDesc:
    video_format_meta_ = std::make_unique<VideoFormatDescMeta>();
    break;
  case BufferType::kVideoPacket:
    video_packet_meta_ = std::make_unique<VideoPacketMeta>();
    break;
  case BufferType::kAudioPacket:
    audio_packet_meta_ = std::make_unique<AudioPacketMeta>();
    break;
  default:
    break;
  }
}

bool Buffer::append_buffer(const uint8_t *data, size_t size) {
  if (size_ + size > capacity_) {
    return false;
  }
  std::copy(data, data + size, data_ + size_);
  size_ += size;
  return true;
}

Buffer::~Buffer() {
  if (own_data_) {
    delete[] data_;
  }
}

BufferType Buffer::get_type() const { return type_; }
VideoFrameMeta *Buffer::get_video_frame_meta() const {
  return video_frame_meta_.get();
}

AudioFrameMeta *Buffer::get_audio_frame_meta() const {
  return audio_frame_meta_.get();
}

VideoFormatDescMeta *Buffer::get_video_format_desc_meta() const {
  return video_format_meta_.get();
}

AudioPacketMeta *Buffer::get_audio_packet_meta() const {
  return audio_packet_meta_.get();
}

VideoPacketMeta *Buffer::get_video_packet_meta() const {
  return video_packet_meta_.get();
}

uint8_t *Buffer::get_data() const { return data_; }

uint8_t *Buffer::get_away_data() {
  if (own_data_ && size_ > 0) {
    own_data_ = false;
    return data_;
  }
  return nullptr;
}

size_t Buffer::get_size() const { return size_; }

} // namespace reddecoder
