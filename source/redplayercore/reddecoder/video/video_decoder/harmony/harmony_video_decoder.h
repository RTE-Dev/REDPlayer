#pragma once

#include <queue>

#include "multimedia/player_framework/native_avcodec_videodecoder.h"
#include "multimedia/player_framework/native_avformat.h"

#include "reddecoder/video/video_codec_info.h"
#include "reddecoder/video/video_common_definition.h"
#include "reddecoder/video/video_decoder/video_decoder.h"

namespace reddecoder {
struct HarmonyMediaFormatReleaser {
  void operator()(OH_AVFormat *ptr) const { OH_AVFormat_Destroy(ptr); }
};

struct HarmonyVideoDecoderReleaser {
  void operator()(OH_AVCodec *ptr) const { OH_VideoDecoder_Destroy(ptr); }
};

struct HarmonyVideoDecoderInputBuffer {
  uint32_t index;
  OH_AVMemory *data;
};

struct HarmonyVideoDecoderOutputBuffer {
  uint32_t index;
  OH_AVCodecBufferAttr attr;
  OH_AVMemory *data;
};

// During Decoder lifecycle, native window must not be destroyed
// Before destroy video decoder, should destroy all the buffers

class BsfCtx {
public:
  int nal_size_len_ = 0;
  int new_idr_ = 0;
  int sps_seen_ = 0;
  int pps_seen_ = 0;
  int extra_data_size_ = 0;
  uint8_t *extra_data_ = nullptr;
  int sps_size_ = 0;
  uint8_t *sps_ = nullptr;
  int pps_size_ = 0;
  uint8_t *pps_ = nullptr;
  int vps_size_ = 0;
  uint8_t *vps_ = nullptr;

  BsfCtx();

  ~BsfCtx();

  void release();

  int init_from_h264(const uint8_t *p_buf, size_t p_buf_size);
  int init_from_h265(const uint8_t *p_buf, size_t p_buf_size);
  void convert_h264(uint8_t *p_buf, size_t p_buf_size, uint8_t **p_out_buf,
                    size_t *p_out_len);
  void convert_h265(uint8_t *p_buf, size_t p_buf_size, uint8_t **p_out_buf,
                    size_t *p_out_len);
};

class HarmonyVideoDecoder : public VideoDecoder {
public:
  HarmonyVideoDecoder(VideoCodecInfo codec);
  ~HarmonyVideoDecoder() override;

  VideoCodecError init(const Buffer *buffer) override;
  /* invalid all the buffer befor release decoder */
  VideoCodecError release() override;
  VideoCodecError decode(const Buffer *buffer) override;
  VideoCodecError flush() override;
  VideoCodecError set_video_format_description(const Buffer *buffer) override;
  VideoCodecError get_delayed_frames() override;
  VideoCodecError get_delayed_frame() override;
  VideoCodecError set_hardware_context(HardWareContext *context) override;
  VideoCodecError update_hardware_context(HardWareContext *context) override;

  int get_serial();
  void on_error_callback(int32_t error);

  std::mutex input_que_mutex_;
  std::mutex output_que_mutex_;
  std::condition_variable not_empty_cv_;
  std::queue<HarmonyVideoDecoderInputBuffer> input_buffer_que_;
  std::queue<HarmonyVideoDecoderOutputBuffer> output_buffer_que_;

private:
  VideoCodecError init_video_decoder();
  VideoCodecError release_video_decoder();

  VideoCodecError init_media_format_desc(const Buffer *buffer);
  VideoCodecError release_media_format_desc();

  std::unique_ptr<OH_AVFormat, HarmonyMediaFormatReleaser> media_format_;
  std::unique_ptr<OH_AVCodec, HarmonyVideoDecoderReleaser> video_decoder_;

  VideoCodecError feed_decoder(const Buffer *buffer, size_t &offset);
  VideoCodecError drain_decoder();

  int current_input_buffer_ = -1;
  CodecContext codec_ctx_;
  BsfCtx bsf_ctx_;
  OHNativeWindow *native_window_{nullptr};
  std::atomic_int serial_;
  bool is_eof_state_ = false;
  bool is_drain_state_ = false;
  bool output_first_frame_state_ = false;
  bool is_decoder_start_ = false;
};

} // namespace reddecoder
