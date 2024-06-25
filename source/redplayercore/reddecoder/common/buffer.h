#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "reddecoder/audio/audio_common_definition.h"
#include "reddecoder/video/video_common_definition.h"

namespace reddecoder {

enum class BufferType {
  kUnknown = 0,
  kAudioFrame = 1,
  kVideoFrame = 2,
  kVideoFormatDesc = 3,
  kAudioPacket = 4,
  kVideoPacket = 5,
};

struct MediaCodecBufferContext {
  int buffer_index;
  void *media_codec;
  int decoder_serial;
  void *decoder;
  void (*release_output_buffer)(MediaCodecBufferContext *context, bool render);
};

struct HarmonyVideoDecoderBufferContext {
  int buffer_index;
  void *video_decoder;
  int decoder_serial;
  void *decoder;
  void (*release_output_buffer)(HarmonyVideoDecoderBufferContext *context,
                                bool render);
};

struct FFmpegBufferContext {
  void *av_frame;
  void (*release_av_frame)(FFmpegBufferContext *context);
};

struct VideoToolBufferContext {
  void *buffer = nullptr;
};

struct VideoFrameMeta {
  int height;
  int width;

  PixelFormat pixel_format;
  int64_t start_decode_ms;
  int64_t end_decode_ms;
  int64_t pts_ms;
  int64_t dts_ms;

  // feild will be set only when pixel format is YUV420P
  int yStride; // stride of Y data buffer
  int uStride; // stride of U data buffer
  int vStride; // stride of V data buffer
  uint8_t *yBuffer = nullptr;
  uint8_t *uBuffer = nullptr;
  uint8_t *vBuffer = nullptr;

  // buffer need release
  void *buffer_context = nullptr;
};

enum class VideoPacketFormat {
  kUnknown = 0,
  kAVCC = 1,
  kAnnexb = 2,
  kFollowExtradataFormat = 3,
};

enum class DecodeFlag {
  kDoNotOutputFrame = 1 << 0,
};

struct VideoPacketMeta {
  uint32_t decode_flags;
  VideoPacketFormat format;
  int64_t pts_ms;
  int64_t dts_ms;
  int offset = 0;
};

struct AudioPacketMeta {
  int num_channels;
  int sample_rate;
  int num_samples;
  int64_t pts_ms;
};

struct VideoFormatDescMetaItem {
  int start_index = 0;
  size_t len = 0;
  VideoFormatDescType type = VideoFormatDescType::kUnknow;
};

enum class VideoColorPrimaries {
  kUnknown = 0,
  kBT2020 = 1,
  kBT709 = 2,
  kSMPTE170M = 3,
  kSMPTE240M = 4,
  kBT470BG = 5,
  kSMPTE432 = 6,
};

enum class VideoColorTransferCharacteristic {
  kUnknown = 0,
  kSMPTEST2084 = 1,
  kARIB_STD_B67 = 2,
  kBT709 = 3,
  kBT2020_10 = 4,
  kBT2020_12 = 5,
  kSMPTE170M = 6,
  kSMPTE240M = 7,
};

enum class VideoColorSpace {
  kUnknown = 0,
  kBT2020_NCL = 1,
  kBT2020_CL = 2,
  kBT709 = 3,
  kSMPTE170M = 4,
  kBT470BG = 5,
  kSMPTE240M = 6,
};

enum class VideoColorRange {
  kUnknown = 0,
  kMPEG = 1, ///< the normal 219*2^(n-8) "MPEG" YUV ranges
  kJPEG = 2, ///< the normal     2^n-1   "JPEG" YUV ranges
  kNB        ///< Not part of ABI
};

enum class VideoDiscard {
  kDISCARD_NONE = 0,    ///< discard nothing
  kDISCARD_DEFAULT = 1, ///< discard useless packets like 0 size packets in avi
  kVDISCARD_NONREF = 2, ///< discard all non reference
  kVDISCARD_BIDIR = 3,  ///< discard all bidirectional frames
  kVDISCARD_NONINTRA = 4, ///< discard all non intra frames
  kVDISCARD_NONKEY = 5,   ///< discard all frames except keyframes
  kVDISCARD_ALL = 6,      ///< discard all
};

struct VideoSampleAspectRatio {
  int num = 0;
  int den = 0;
};

struct VideoFormatDescMeta {
  int width;
  int height;
  bool is_hdr = false;

  ~VideoFormatDescMeta() {
    if (hardware_context) {
      delete hardware_context;
      hardware_context = nullptr;
    }
  }

  HardWareContext *hardware_context = nullptr;
  std::vector<VideoFormatDescMetaItem> items;

  // optional, degree that output frame should be rotated, now only need in
  // android
  int rotate_degree = 0;

  // optional, now only used by ios videotoolbox
  VideoColorPrimaries color_primaries;
  VideoColorTransferCharacteristic color_trc;
  VideoColorSpace colorspace;
  VideoColorRange color_range;

  // optional, now used for software decode
  VideoDiscard skip_frame = VideoDiscard::kDISCARD_NONE;
  VideoDiscard skip_loop_filter = VideoDiscard::kDISCARD_NONE;
  VideoDiscard skip_idct = VideoDiscard::kDISCARD_NONE;

  VideoSampleAspectRatio sample_aspect_ratio;
};

const int MAX_PALANARS = 8;
struct AudioFrameMeta {
  int num_channels;
  int sample_rate;
  int num_samples;
  int64_t pts_ms;

  int sample_format;
  uint64_t channel_layout;

  int channel_size;
  uint8_t *channel[MAX_PALANARS];
  // buffer need release
  void *buffer_context = nullptr;
};

class Buffer {
public:
  Buffer(BufferType type, uint8_t *data, size_t size, bool own_data);
  Buffer(BufferType type, size_t capacity);
  ~Buffer();

  BufferType get_type() const;
  VideoFrameMeta *get_video_frame_meta() const;
  AudioFrameMeta *get_audio_frame_meta() const;
  VideoFormatDescMeta *get_video_format_desc_meta() const;

  VideoPacketMeta *get_video_packet_meta() const;
  AudioPacketMeta *get_audio_packet_meta() const;

  uint8_t *get_data() const;
  uint8_t *get_away_data();
  size_t get_size() const;

  bool append_buffer(const uint8_t *data, size_t size);
  void replace_buffer(uint8_t *data, size_t size, bool own_data);

private:
  void init_type(BufferType type);
  BufferType type_;
  std::unique_ptr<VideoFrameMeta> video_frame_meta_;
  std::unique_ptr<AudioFrameMeta> audio_frame_meta_;
  std::unique_ptr<VideoPacketMeta> video_packet_meta_;
  std::unique_ptr<AudioPacketMeta> audio_packet_meta_;
  std::unique_ptr<VideoFormatDescMeta> video_format_meta_;
  uint8_t *data_;
  size_t size_;
  size_t capacity_;
  bool own_data_;
};
} // namespace reddecoder
