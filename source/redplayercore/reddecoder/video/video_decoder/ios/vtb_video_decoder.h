#pragma once

#include <VideoToolbox/VideoToolbox.h>

#include <queue>

#include "reddecoder/video/video_codec_info.h"
#include "reddecoder/video/video_common_definition.h"
#include "reddecoder/video/video_decoder/video_decoder.h"

namespace reddecoder {

struct CmpBufferByPts {
  bool operator()(Buffer *a, Buffer *b) {
    if (a && b && a->get_type() == BufferType::kVideoFrame &&
        b->get_type() == BufferType::kVideoFrame) {
      return a->get_video_frame_meta()->pts_ms >
             b->get_video_frame_meta()->pts_ms;
    }
    return false;
  }
};

typedef std::priority_queue<Buffer *, std::vector<Buffer *>, CmpBufferByPts>
    BufferQueue;

class VtbVideoDecoder : public VideoDecoder {
public:
  struct SampleInfo {
    int64_t pts_ms;
    int64_t dts_ms;
  };
  VtbVideoDecoder(VideoCodecInfo codec);
  ~VtbVideoDecoder() override;
  VideoCodecError init(const Buffer *buffer) override;
  VideoCodecError release() override;
  VideoCodecError decode(const Buffer *buffer) override;
  VideoCodecError flush() override;
  VideoCodecError set_video_format_description(const Buffer *buffer) override;
  VideoCodecError get_delayed_frames() override;
  VideoCodecError get_delayed_frame() override;
  BufferQueue &get_buffer_queue() {
    BufferQueue &ref = buffer_queue_;
    return ref;
  }
  int get_max_ref_frames() { return max_ref_frames_; }
  SampleInfo get_sample_info() { return sample_info_; }

private:
  VideoCodecError init_vt_decompress_session();
  VideoCodecError release_vt_decompress_seesion();

  VideoCodecError init_cm_format_desc(const Buffer *buffer);
  VideoCodecError release_cm_format_desc();

  CMSampleBufferRef create_cm_sample_buffer(void *data, size_t size);
  VideoCodecError
  init_cm_format_desc_color_space_info(CFMutableDictionaryRef extensions,
                                       VideoFormatDescMeta *meta);
  CMVideoFormatDescriptionRef cm_format_desc_ = nullptr;
  VTDecompressionSessionRef vt_decompress_session_ = nullptr;
  int max_ref_frames_ = 0;
  BufferQueue buffer_queue_;
  SampleInfo sample_info_;
  CodecContext codec_ctx_;

public:
  int continue_pts_reorder_error_ = 0;
  std::mutex buffer_queue_mutex_;
};

} // namespace reddecoder
