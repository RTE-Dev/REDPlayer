#include "reddecoder/video/video_decoder/harmony/harmony_video_decoder.h"

#include "bits/alltypes.h"
#include "hilog/log.h"
#include "multimedia/player_framework/native_avcodec_base.h"
#include "native_window/external_window.h"
#include "sys/types.h"

#include <memory>

#include "reddecoder/common/logger.h"
#include "reddecoder/video/video_common/format_convert_helper.h"

extern "C" {
#include "libavutil/avutil.h"
}
namespace reddecoder {

static void count_or_copy(uint8_t **out, uint64_t *out_size, const uint8_t *in,
                          int in_size, int ps, int copy) {
  uint8_t start_code_size = ps < 0 ? 0 : *out_size == 0 || ps ? 4 : 3;
  if (copy) {
    memcpy(*out + start_code_size, in, in_size);
    if (start_code_size == 4) {
      (*out)[0] = (*out)[1] = (*out)[2] = 0;
      (*out)[3] = 1;
    } else if (start_code_size) {
      (*out)[0] = (*out)[1] = 0;
      (*out)[2] = 1;
    }
    *out += start_code_size + in_size;
  }
  *out_size += start_code_size + in_size;
}

BsfCtx::BsfCtx() {}

BsfCtx::~BsfCtx() { release(); }

void BsfCtx::release() {
  if (extra_data_ != nullptr) {
    free(extra_data_);
    extra_data_ = nullptr;
    extra_data_size_ = 0;
  }
  sps_ = nullptr;
  sps_size_ = 0;
  pps_ = nullptr;
  pps_size_ = 0;
  vps_ = nullptr;
  vps_size_ = 0;
}

int BsfCtx::init_from_h264(const uint8_t *p_buf, size_t p_buf_size) {
  release();

  if (p_buf_size == 0 ||
      (p_buf_size == 3 && p_buf[0] == 0 && p_buf[1] == 0 && p_buf[2] == 1) ||
      (p_buf_size == 4 && p_buf[0] == 0 && p_buf[1] == 0 && p_buf[2] == 0 &&
       p_buf[3] == 1)) {
    AV_LOGE(DEC_TAG, "The input looks like it is Annex B already\n");
    return -1;
  }

  if (p_buf_size < 7) {
    AV_LOGE(DEC_TAG, "Invalid extradata size: %zu\n", p_buf_size);
    return -1;
  }

  uint8_t *buf_temp = nullptr;
  int nal_size = (p_buf[4] & 0x03) + 1;
  p_buf += 5;
  p_buf_size -= 5;

  int unit_size = 0;
  int total_size = 0;
  int pps_offset = 0;
  int sps_done = 0;
  int uint_nb = p_buf[0] & 0x1f;
  p_buf++;
  p_buf_size--;
  if (uint_nb == 0) {
    goto pps;
  }

  while (uint_nb--) {
    if (p_buf_size < 2) {
      return -1;
    }

    unit_size = (p_buf[0] << 8) | p_buf[1];
    p_buf += 2;
    p_buf_size -= 2;

    total_size += unit_size + 4;
    if (p_buf_size < unit_size + !sps_done) {
      return -1;
    }

    buf_temp = reinterpret_cast<uint8_t *>(malloc(total_size));
    memcpy(buf_temp + total_size - unit_size, p_buf, unit_size);
    buf_temp[total_size - unit_size - 4] = 0;
    buf_temp[total_size - unit_size - 3] = 0;
    buf_temp[total_size - unit_size - 2] = 0;
    buf_temp[total_size - unit_size - 1] = 1;

    if (extra_data_ != nullptr) {
      memcpy(buf_temp, extra_data_, total_size - unit_size - 4);
      free(extra_data_);
      extra_data_ = nullptr;
      extra_data_size_ = 0;
    }

    extra_data_ = buf_temp;
    extra_data_size_ = total_size;
    p_buf += unit_size;
    p_buf_size -= unit_size;

  pps:
    if (!uint_nb && !sps_done++) {
      if (p_buf_size < 1) {
        return -1;
      }

      uint_nb = p_buf[0];
      p_buf++;
      p_buf_size--;
      pps_offset = total_size;
    }
  }

  nal_size_len_ = nal_size;
  new_idr_ = 1;
  sps_seen_ = 0;
  pps_seen_ = 0;
  sps_ = extra_data_;
  sps_size_ = pps_offset;
  pps_ = extra_data_ + pps_offset;
  pps_size_ = extra_data_size_ - pps_offset;
  return 0;
}

void BsfCtx::convert_h264(uint8_t *p_buf, size_t i_len, uint8_t **p_out_buf,
                          size_t *p_out_len) {
  uint8_t *buf = p_buf;
  uint8_t *buf_end = p_buf + i_len;
  uint8_t *out_buf = nullptr;
  uint64_t out_buf_size = 0;

  const int MPEG_NAL_SPS = 7;
  const int MPEG_NAL_PPS = 8;
  const int MPEG_NAL_IDR_SLICE = 5;
  const int MPEG_NAL_SLICE = 1;
  int sps_seen = 0;
  int pps_seen = 0;
  int new_idr = 0;
  int unit_type = 0;
  for (int copy = 0; copy < 2; copy++) {
    sps_seen = sps_seen_;
    pps_seen = pps_seen_;
    new_idr = new_idr_;
    buf = p_buf;
    out_buf_size = 0;
    do {
      int nal_size = 0;
      for (int i = 0; i < nal_size_len_; i++) {
        nal_size = (nal_size << 8) | buf[i];
      }
      buf += nal_size_len_;
      if (buf + nal_size > buf_end) {
        goto fail;
      }

      if (nal_size == 0) {
        continue;
      }

      unit_type = (*buf) & 0x1f;
      if (unit_type == MPEG_NAL_SPS) {
        sps_seen = new_idr = 1;
      }

      if (unit_type == MPEG_NAL_PPS) {
        pps_seen = new_idr = 1;
        if (!sps_seen) {
          if (!sps_size_) {
            AV_LOGE(DEC_TAG, "SPS not present in the stream, nor in AVCC, "
                             "stream may be unreadable\n");
          } else {
            count_or_copy(&out_buf, &out_buf_size, sps_, sps_size_, -1, copy);
            sps_seen = 1;
          }
        }
      }

      /* If this is a new IDR picture following an IDR picture, reset the idr
       * flag. Just check first_mb_in_slice to be 0 as this is the simplest
       * solution. This could be checking idr_pic_id instead, but would
       * complexify the parsing. */
      if (!new_idr && unit_type == MPEG_NAL_IDR_SLICE && (buf[1] & 0x80)) {
        new_idr = 1;
      }

      /* prepend only to the first type 5 NAL unit of an IDR picture, if no
       * sps/pps are already present */
      if (new_idr && unit_type == MPEG_NAL_IDR_SLICE && !sps_seen &&
          !pps_seen) {
        if (extra_data_) {
          count_or_copy(&out_buf, &out_buf_size, extra_data_, extra_data_size_,
                        -1, copy);
        }
        new_idr = 0;
        /* if only SPS has been seen, also insert PPS */
      } else if (new_idr && unit_type == MPEG_NAL_IDR_SLICE && sps_seen &&
                 !pps_seen) {
        if (!pps_size_) {
          AV_LOGE(DEC_TAG,
                  "PPS not present in the stream, nor in AVCC, stream may be "
                  "unreadable\n");
        } else {
          count_or_copy(&out_buf, &out_buf_size, pps_, pps_size_, -1, copy);
        }
      }

      count_or_copy(&out_buf, &out_buf_size, buf, nal_size,
                    unit_type == MPEG_NAL_PPS || unit_type == MPEG_NAL_SPS,
                    copy);
      if (!new_idr && unit_type == MPEG_NAL_SLICE) {
        new_idr = 1;
        sps_seen = 0;
        pps_seen = 0;
      }
      buf += nal_size;
    } while (buf != buf_end);

    if (!copy) {
      if (out_buf_size > INT_MAX) {
        return;
      }

      out_buf = new uint8_t[out_buf_size];
      if (out_buf == nullptr) {
        goto fail;
      }

      *p_out_buf = out_buf;
      *p_out_len = out_buf_size;
    }
  }

  new_idr_ = new_idr;
  sps_seen_ = sps_seen;
  pps_seen_ = pps_seen;
fail : {}
}

int BsfCtx::init_from_h265(const uint8_t *p_buf, size_t p_buf_size) {
  release();

  if (p_buf_size == 0 ||
      (p_buf_size == 3 && p_buf[0] == 0 && p_buf[1] == 0 && p_buf[2] == 1) ||
      (p_buf_size == 4 && p_buf[0] == 0 && p_buf[1] == 0 && p_buf[2] == 0 &&
       p_buf[3] == 1)) {
    AV_LOGE(DEC_TAG, "The input looks like it is Annex B already\n");
    return -1;
  }

  if (p_buf_size < 23) {
    AV_LOGE(DEC_TAG, "Invalid extradata size: %zu\n", p_buf_size);
    return -1;
  }

  uint8_t *buf_temp = nullptr;
  p_buf += 21;
  p_buf_size -= 21;

  int nal_size = (p_buf[0] & 0x03) + 1;
  p_buf++;
  p_buf_size--;

  int total_size = 0;

  int unit_nb = p_buf[0] & 0x1f;
  p_buf++;
  p_buf_size--;

  for (int i = 0; i < unit_nb; i++) {
    if (p_buf_size < 3) {
      return -1;
    }

    int cnt = (p_buf[1] << 8) | p_buf[2];
    p_buf += 3;
    p_buf_size -= 3;

    for (int j = 0; j < cnt; j++) {
      if (p_buf_size < 2) {
        return -1;
      }

      int unit_size = (p_buf[0] << 8) | p_buf[1];
      p_buf += 2;
      p_buf_size -= 2;

      if (p_buf_size < nal_size) {
        return -1;
      }

      total_size += unit_size + 4;
      buf_temp = reinterpret_cast<uint8_t *>(malloc(total_size));
      memcpy(buf_temp + total_size - unit_size, p_buf, unit_size);
      buf_temp[total_size - unit_size - 4] = 0;
      buf_temp[total_size - unit_size - 3] = 0;
      buf_temp[total_size - unit_size - 2] = 0;
      buf_temp[total_size - unit_size - 1] = 1;

      if (extra_data_ != nullptr) {
        memcpy(buf_temp, extra_data_, total_size - unit_size - 4);
        free(extra_data_);
        extra_data_ = nullptr;
        extra_data_size_ = 0;
      }

      extra_data_ = buf_temp;
      extra_data_size_ = total_size;
      p_buf += unit_size;
      p_buf_size -= unit_size;
    }
  }

  nal_size_len_ = nal_size;
  return 0;
}

void BsfCtx::convert_h265(uint8_t *p_buf, size_t i_len, uint8_t **p_out_buf,
                          size_t *p_out_len) {
  uint8_t *buf = p_buf;
  uint8_t *buf_end = p_buf + i_len;
  uint8_t *out_buf = nullptr;
  uint64_t out_buf_size = 0;
  int got_irap = 0;

  for (int copy = 0; copy < 2; copy++) {
    buf = p_buf;
    out_buf_size = 0;
    got_irap = 0;
    do {
      int nal_size = 0;
      for (int i = 0; i < nal_size_len_; i++) {
        nal_size = (nal_size << 8) | buf[i];
      }
      buf += nal_size_len_;

      if (buf + nal_size > buf_end) {
        return;
      }

      if (nal_size == 0) {
        continue;
      }

      int unit_type = ((*buf) >> 1) & 0x1f;
      int is_irap = (unit_type >= 16) && (unit_type <= 23);
      int add_extradata = is_irap && (!got_irap);
      got_irap |= is_irap;
      if (add_extradata) {
        out_buf_size += extra_data_size_;
        if (copy) {
          memcpy(out_buf, extra_data_, extra_data_size_);
          out_buf += extra_data_size_;
        }
      }

      count_or_copy(&out_buf, &out_buf_size, buf, nal_size, 0, copy);
      buf += nal_size;
    } while (buf != buf_end);

    if (!copy) {
      if (out_buf_size > INT_MAX) {
        return;
      }

      out_buf = new uint8_t[out_buf_size];
      if (out_buf == nullptr) {
        return;
      }

      *p_out_buf = out_buf;
      *p_out_len = out_buf_size;
    }
  }
}

static void
oh_videodecoder_buffer_release(HarmonyVideoDecoderBufferContext *context,
                               bool render) {
  if (context && context->video_decoder && context->decoder) {
    if (context->decoder_serial ==
        (reinterpret_cast<HarmonyVideoDecoder *>(context->decoder))
            ->get_serial()) {
      if (render) {
        OH_VideoDecoder_RenderOutputData(
            reinterpret_cast<OH_AVCodec *>(context->video_decoder),
            context->buffer_index);
      } else {
        OH_VideoDecoder_FreeOutputData(
            reinterpret_cast<OH_AVCodec *>(context->video_decoder),
            context->buffer_index);
      }
    } else {
      OH_VideoDecoder_FreeOutputData(
          reinterpret_cast<OH_AVCodec *>(context->video_decoder),
          context->buffer_index);
      AV_LOGW(DEC_TAG,
              "[reddecoder] release buffer serial is not equal, index:%d "
              "serial:%d\n",
              context->buffer_index, context->decoder_serial);
    }
  }
}

static void OH_VideoDecoderOnNeedInputDataCallback(OH_AVCodec *codec,
                                                   uint32_t index,
                                                   OH_AVMemory *data,
                                                   void *userData) {
  if (!userData) {
    return;
  }
  HarmonyVideoDecoder *decoder =
      reinterpret_cast<HarmonyVideoDecoder *>(userData);
  std::unique_lock<std::mutex> in_lock(decoder->input_que_mutex_);
  HarmonyVideoDecoderInputBuffer input_buffer;
  input_buffer.index = index;
  input_buffer.data = data;
  decoder->input_buffer_que_.push(input_buffer);
  decoder->not_empty_cv_.notify_all();
}

static void OH_VideoDecoderOnStreamChanged(OH_AVCodec *codec,
                                           OH_AVFormat *format,
                                           void *userData) {
  AV_LOGW(DEC_TAG, "[reddecoder] OH_VideoDecoderOnStreamChanged ");
}

static void OH_VideoDecoderOnError(OH_AVCodec *codec, int32_t errorCode,
                                   void *userData) {
  AV_LOGE(DEC_TAG, "[reddecoder] OH_VideoDecoderOnError %d", errorCode);
  if (!userData) {
    return;
  }
  HarmonyVideoDecoder *decoder =
      reinterpret_cast<HarmonyVideoDecoder *>(userData);
  decoder->on_error_callback(errorCode);
}

static void OH_VideoDecoderOnNeedOutputDataCallback(OH_AVCodec *codec,
                                                    uint32_t index,
                                                    OH_AVMemory *data,
                                                    OH_AVCodecBufferAttr *attr,
                                                    void *userData) {
  if (!userData) {
    return;
  }
  HarmonyVideoDecoder *decoder =
      reinterpret_cast<HarmonyVideoDecoder *>(userData);
  std::unique_lock<std::mutex> out_lock(decoder->output_que_mutex_);
  HarmonyVideoDecoderOutputBuffer output_buffer;
  output_buffer.index = index;
  output_buffer.data = data;

  memcpy(&(output_buffer.attr), attr, sizeof(OH_AVCodecBufferAttr));
  decoder->output_buffer_que_.push(output_buffer);
}

HarmonyVideoDecoder::HarmonyVideoDecoder(VideoCodecInfo codec)
    : VideoDecoder(codec) {
  std::atomic_init(&serial_, 1);
}

HarmonyVideoDecoder::~HarmonyVideoDecoder() { release(); }

VideoCodecError HarmonyVideoDecoder::init(const Buffer *buffer) {
  if ((codec_info_.codec_name != VideoCodecName::kH264 &&
       codec_info_.codec_name != VideoCodecName::kH265) ||
      codec_info_.implementation_type !=
          VideoCodecImplementationType::kHardwareHarmony) {
    return VideoCodecError::kInitError;
  }
  release();

  if (buffer) {
    set_hardware_context(
        buffer->get_video_format_desc_meta()->hardware_context);
    if (buffer->get_size() > 0) {
      init_media_format_desc(buffer);
      init_video_decoder();
    }
  }
  AV_LOGI(DEC_TAG, "[reddecoder_%p] init \n", this);
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::release() {
  release_video_decoder();
  release_media_format_desc();

  if (native_window_) {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] %s release old native_window %p\n", this,
            __func__, native_window_);
    OH_NativeWindow_NativeObjectUnreference(native_window_);
    native_window_ = nullptr;
  }

  AV_LOGI(DEC_TAG, "[reddecoder_%p] release \n", this);
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::feed_decoder(const Buffer *buffer,
                                                  size_t &offset) {
  VideoPacketMeta *meta = buffer->get_video_packet_meta();
  while ((offset < buffer->get_size() || buffer->get_size() == 0) &&
         !is_drain_state_) {
    std::unique_lock<std::mutex> in_lock(input_que_mutex_);

    HarmonyVideoDecoderInputBuffer input_buffer;
    if (input_buffer_que_.empty()) {
      return VideoCodecError::kTryAgain;
    }
    input_buffer = input_buffer_que_.front();
    int32_t buf_size = OH_AVMemory_GetSize(input_buffer.data);
    uint8_t *buf = OH_AVMemory_GetAddr(input_buffer.data);
    if (buf_size <= 0 || !buf) {
      AV_LOGE(DEC_TAG,
              "[reddecoder] oh video decoder get input buffer error: %d\n",
              static_cast<int>(buf_size));
      return VideoCodecError::kInternalError;
    }

    size_t copy_size = (buffer->get_size() - offset) < buf_size
                           ? (buffer->get_size() - offset)
                           : buf_size;

    if (buffer->get_size() == 0) {
      is_drain_state_ = true;
      AV_LOGI(
          DEC_TAG,
          "[reddecoder] oh video decoder input end of stream copy_size:%zu, "
          "buffer_size:%zu, buffer_size:%" PRId32 ", offset:%zu\n",
          copy_size, buffer->get_size(), buf_size, offset);
    }

    if (copy_size > 0 && copy_size <= buf_size &&
        (offset + copy_size <= buffer->get_size())) {
      memcpy(buf, buffer->get_data() + offset, copy_size);
      offset += copy_size;
    } else {
      if (buffer->get_size() != 0) {
        AV_LOGE(DEC_TAG,
                "[reddecoder] error during memcpy, copy_size: %zu, "
                "buffer_size:%zu, buffer_size:%" PRId32 ", offset:%zu\n",
                copy_size, buffer->get_size(), buf_size, offset);
        return VideoCodecError::kInternalError;
      }
    }

    int64_t pts = meta->pts_ms;
    if (pts == AV_NOPTS_VALUE && meta->dts_ms != AV_NOPTS_VALUE) {
      pts = meta->dts_ms;
    }
    if (pts >= 0) {
      pts = pts * 1000;
    } else {
      pts = 0;
    }

    OH_AVCodecBufferAttr attr;
    attr.pts = pts;
    attr.size = copy_size;
    attr.offset = 0;
    attr.flags = is_drain_state_ ? AVCODEC_BUFFER_FLAGS_EOS : 0;

    OH_AVErrCode status = OH_VideoDecoder_PushInputData(
        video_decoder_.get(), input_buffer.index, attr);
    if (status != AV_ERR_OK) {
      AV_LOGE(DEC_TAG,
              "[reddecoder] oh video decoder push input buffer error: %d\n",
              static_cast<int>(status));
      return VideoCodecError::kInternalError;
    }
    input_buffer_que_.pop();
  }
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::drain_decoder() {
  if (!is_eof_state_) {
    std::unique_lock<std::mutex> out_lock(output_que_mutex_);
    if (!output_buffer_que_.empty()) {
      HarmonyVideoDecoderOutputBuffer output_buffer =
          output_buffer_que_.front();

      std::unique_ptr<Buffer> buffer =
          std::make_unique<Buffer>(BufferType::kVideoFrame, 0);

      VideoFrameMeta *meta = buffer->get_video_frame_meta();
      if (codec_ctx_.rotate_degree == 90 || codec_ctx_.rotate_degree == 270) {
        meta->height = codec_ctx_.width;
        meta->width = codec_ctx_.height;
      } else {
        meta->height = codec_ctx_.height;
        meta->width = codec_ctx_.width;
      }

      meta->pixel_format = PixelFormat::kHarmonyVideoDecoderBuffer;
      meta->pts_ms = output_buffer.attr.pts / 1000;
      if (meta->pts_ms < 0) {
        meta->pts_ms = AV_NOPTS_VALUE;
      }
      meta->buffer_context =
          reinterpret_cast<void *>(new HarmonyVideoDecoderBufferContext{
              .buffer_index = static_cast<int>(output_buffer.index),
              .video_decoder = video_decoder_.get(),
              .decoder_serial = serial_.load(),
              .decoder = this,
              .release_output_buffer = oh_videodecoder_buffer_release,
          });

      if (output_buffer.attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        AV_LOGI(DEC_TAG, "[reddecoder] output end of stream %d\n",
                output_buffer.attr.flags);
        is_eof_state_ = true;
        return VideoCodecError::kEOF;
      }

      if (!output_first_frame_state_) {
        output_first_frame_state_ = true;
      }

      video_decoded_callback_->on_decoded_frame(std::move(buffer));
      output_buffer_que_.pop();
    } else {
      if (is_drain_state_) {
        AV_LOGE(DEC_TAG, "[reddecoder] oh video decoder return eof due to "
                         "drain process error");
        return VideoCodecError::kEOF;
      }
    }
  } else {
    return VideoCodecError::kEOF;
  }
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::decode(const Buffer *buffer) {
  if (!buffer || buffer->get_type() != BufferType::kVideoPacket) {
    return VideoCodecError::kInvalidParameter;
  }

  if (!media_format_ || !video_decoder_) {
    return VideoCodecError::kInitError;
  }

  if (buffer->get_video_packet_meta()->format == VideoPacketFormat::kAVCC ||
      (buffer->get_video_packet_meta()->format ==
           VideoPacketFormat::kFollowExtradataFormat &&
       !codec_ctx_.is_annexb_extradata)) {
    uint8_t *buffer_convert = nullptr;
    size_t buffer_convert_size = 0;

    if (codec_info_.codec_name == VideoCodecName::kH264) {
      bsf_ctx_.convert_h264(buffer->get_data(), buffer->get_size(),
                            &buffer_convert, &buffer_convert_size);
    } else if (codec_info_.codec_name == VideoCodecName::kH265) {
      bsf_ctx_.convert_h265(buffer->get_data(), buffer->get_size(),
                            &buffer_convert, &buffer_convert_size);
    }
    const_cast<Buffer *>(buffer)->replace_buffer(buffer_convert,
                                                 buffer_convert_size, true);
    buffer->get_video_packet_meta()->format =
        reddecoder::VideoPacketFormat::kAnnexb;
  }

  size_t offset = 0;
  if (buffer->get_video_packet_meta()->offset > 0) {
    offset = buffer->get_video_packet_meta()->offset;
  }

  while (1) {
    VideoCodecError ret = drain_decoder();
    if (ret == VideoCodecError::kEOF) {
      return ret;
    }

    ret = feed_decoder(buffer, offset);
    if (ret == VideoCodecError::kTryAgain) {
      std::unique_lock<std::mutex> in_lock(input_que_mutex_);
      not_empty_cv_.wait_for(in_lock, std::chrono::milliseconds(5),
                             [this] { return input_buffer_que_.size() > 0; });
      continue;
    } else if (ret == VideoCodecError::kNoError) {
      if (offset < buffer->get_size()) {
        continue;
      } else {
        return VideoCodecError::kNoError;
      }
    } else {
      return ret;
    }
  }
}

VideoCodecError
HarmonyVideoDecoder::init_media_format_desc(const Buffer *buffer) {
  if (!buffer || buffer->get_type() != BufferType::kVideoFormatDesc) {
    return VideoCodecError::kInvalidParameter;
  }

  VideoFormatDescMeta *meta = buffer->get_video_format_desc_meta();
  if (!meta || meta->height <= 0 || meta->width <= 0) {
    return VideoCodecError::kInvalidParameter;
  }

  uint8_t *extradata = buffer->get_data();
  size_t extradata_size = buffer->get_size();

  if (extradata_size < 7 || !extradata) {
    return VideoCodecError::kInvalidParameter;
  }

  codec_ctx_.is_annexb_extradata = false;

  release_media_format_desc();
  media_format_.reset(OH_AVFormat_Create());

  if (extradata[0] == 1 || extradata[1] == 1) {
    if (codec_info_.codec_name == VideoCodecName::kH264) {
      if ((bsf_ctx_.init_from_h264(extradata, extradata_size)) != 0) {
        return VideoCodecError::kInitError;
      }
    } else if (codec_info_.codec_name == VideoCodecName::kH265) {
      if ((bsf_ctx_.init_from_h265(extradata, extradata_size)) != 0) {
        return VideoCodecError::kInitError;
      }
    }
    codec_ctx_.nal_size = bsf_ctx_.nal_size_len_;
  } else {
    codec_ctx_.is_annexb_extradata = true;
  }
  OH_AVFormat_SetBuffer(media_format_.get(), OH_MD_KEY_CODEC_CONFIG,
                        bsf_ctx_.extra_data_, bsf_ctx_.extra_data_size_);

  if (codec_info_.codec_name == VideoCodecName::kH264) {
    OH_AVFormat_SetStringValue(media_format_.get(), OH_MD_KEY_CODEC_MIME,
                               OH_AVCODEC_MIMETYPE_VIDEO_AVC);

    AV_LOGI(DEC_TAG, "[reddecoder_%p] OH_AVFormat_SetStringValue %s %s\n", this,
            OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);

  } else if (codec_info_.codec_name == VideoCodecName::kH265) {
    OH_AVFormat_SetStringValue(media_format_.get(), OH_MD_KEY_CODEC_MIME,
                               OH_AVCODEC_MIMETYPE_VIDEO_HEVC);

    AV_LOGI(DEC_TAG, "[reddecoder_%p] OH_AVFormat_SetStringValue %s %s\n", this,
            OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
  }

  codec_ctx_.width = meta->width;
  codec_ctx_.height = meta->height;
  codec_ctx_.rotate_degree = meta->rotate_degree;

  if (codec_ctx_.rotate_degree != 0) {
    OH_AVFormat_SetIntValue(media_format_.get(), OH_MD_KEY_ROTATION,
                            codec_ctx_.rotate_degree);
  }

  OH_AVFormat_SetIntValue(media_format_.get(), OH_MD_KEY_WIDTH,
                          codec_ctx_.width);
  OH_AVFormat_SetIntValue(media_format_.get(), OH_MD_KEY_HEIGHT,
                          codec_ctx_.height);
  OH_AVFormat_SetIntValue(media_format_.get(), OH_MD_KEY_PIXEL_FORMAT,
                          AV_PIXEL_FORMAT_SURFACE_FORMAT);
  OH_AVFormat_SetIntValue(media_format_.get(), "VideoPixelFormat",
                          AV_PIXEL_FORMAT_RGBA);

  OH_AVFormat_SetIntValue(media_format_.get(), OH_MD_KEY_MAX_INPUT_SIZE, 0);
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::release_media_format_desc() {
  media_format_.reset();
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::init_video_decoder() {
  release_video_decoder();

  if (!media_format_) {
    return VideoCodecError::kUninited;
  }

  current_input_buffer_ = -1;

  if (codec_info_.codec_name == VideoCodecName::kH264) {
    video_decoder_.reset(
        OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC));
  } else if (codec_info_.codec_name == VideoCodecName::kH265) {
    video_decoder_.reset(
        OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
  }

  if (!video_decoder_) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder init error\n", this);
    return VideoCodecError::kInitError;
  } else {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] oh video decoder inited %p\n", this,
            video_decoder_.get());
  }

  OH_AVErrCode status = AV_ERR_OK;

  OH_AVCodecAsyncCallback callback;
  callback.onNeedInputData = OH_VideoDecoderOnNeedInputDataCallback;
  callback.onNeedOutputData = OH_VideoDecoderOnNeedOutputDataCallback;
  callback.onStreamChanged = OH_VideoDecoderOnStreamChanged;
  callback.onError = OH_VideoDecoderOnError;
  status = OH_VideoDecoder_SetCallback(video_decoder_.get(), callback, this);
  if (status != AV_ERR_OK) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder set callback error %u\n",
            this, status);
    return VideoCodecError::kInitError;
  }

  status = OH_VideoDecoder_Configure(video_decoder_.get(), media_format_.get());
  if (status != AV_ERR_OK) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder configure error %u\n",
            this, status);
    return VideoCodecError::kInitError;
  }

  status = OH_VideoDecoder_SetSurface(video_decoder_.get(), native_window_);
  if (status != AV_ERR_OK) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder set surface error %u\n",
            this, status);
    return VideoCodecError::kInitError;
  }

  status = OH_VideoDecoder_Prepare(video_decoder_.get());
  if (status != AV_ERR_OK) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder prepare error %u\n",
            this, status);
    return VideoCodecError::kInitError;
  }

  status = OH_VideoDecoder_Start(video_decoder_.get());
  if (status != AV_ERR_OK) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder start error %u\n", this,
            status);
    return VideoCodecError::kInitError;
  }

  is_decoder_start_ = true;

  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::release_video_decoder() {
  serial_++;
  if (video_decoder_ && is_decoder_start_) {
    OH_AVErrCode ret = OH_VideoDecoder_Stop(video_decoder_.get());
    if (ret != AV_ERR_OK) {
      AV_LOGE(DEC_TAG, "[reddecoder] oh video decoder error %d\n",
              static_cast<int>(ret));
    }
  }
  video_decoder_.reset();
  is_decoder_start_ = false;

  {
    std::unique_lock<std::mutex> in_lock(input_que_mutex_);
    std::queue<HarmonyVideoDecoderInputBuffer> empty_in_que;
    input_buffer_que_.swap(empty_in_que);
  }

  {
    std::unique_lock<std::mutex> out_lock(output_que_mutex_);
    std::queue<HarmonyVideoDecoderOutputBuffer> empty_out_que;
    output_buffer_que_.swap(empty_out_que);
  }

  return VideoCodecError::kNoError;
}

VideoCodecError
HarmonyVideoDecoder::set_video_format_description(const Buffer *buffer) {
  if (buffer->get_type() != BufferType::kVideoFormatDesc ||
      !buffer->get_video_format_desc_meta()) {
    return VideoCodecError::kInvalidParameter;
  }

  HarmonyHardWareContext *ctx = reinterpret_cast<HarmonyHardWareContext *>(
      buffer->get_video_format_desc_meta()->hardware_context);
  if (ctx) {
    native_window_ = reinterpret_cast<OHNativeWindow *>(ctx->natvie_window);
    AV_LOGI(DEC_TAG, "[reddecoder] set native window %p\n", native_window_);
  }

  for (auto &item : buffer->get_video_format_desc_meta()->items) {
    if (item.type == VideoFormatDescType::kExtraData) {
      auto ret = init_media_format_desc(buffer);
      if (ret != VideoCodecError::kNoError) {
        return ret;
      }
      ret = init_video_decoder();
      if (ret != VideoCodecError::kNoError) {
        return ret;
      }
    }
  }
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::flush() {
  serial_++;
  is_drain_state_ = false;
  is_eof_state_ = false;
  output_first_frame_state_ = false;

  if (video_decoder_) {
    OH_AVErrCode status = OH_VideoDecoder_Flush(video_decoder_.get());
    if (status != AV_ERR_OK) {
      AV_LOGE(DEC_TAG, "[reddecoder] oh video decoder flush error: %u\n",
              status);
      return VideoCodecError::kInternalError;
    }
  }

  {
    std::unique_lock<std::mutex> in_lock(input_que_mutex_);
    std::queue<HarmonyVideoDecoderInputBuffer> empty_in_que;
    input_buffer_que_.swap(empty_in_que);
  }

  {
    std::unique_lock<std::mutex> out_lock(output_que_mutex_);
    std::queue<HarmonyVideoDecoderOutputBuffer> empty_out_que;
    output_buffer_que_.swap(empty_out_que);
  }

  if (video_decoder_) {
    OH_AVErrCode status = OH_VideoDecoder_Start(video_decoder_.get());
    if (status != AV_ERR_OK) {
      AV_LOGE(DEC_TAG, "[reddecoder_%p] oh video decoder start error %u\n",
              this, status);
      return VideoCodecError::kInitError;
    }
  }

  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::get_delayed_frames() {
  Buffer buffer(BufferType::kVideoPacket, 0);
  AV_LOGI(DEC_TAG, "[reddecoder] oh video decoder get delayed frames\n");
  while (decode(&buffer) == VideoCodecError::kNoError) {
  }
  is_drain_state_ = false;
  is_eof_state_ = false;
  output_first_frame_state_ = false;
  return VideoCodecError::kNoError;
}

VideoCodecError HarmonyVideoDecoder::get_delayed_frame() {
  Buffer buffer(BufferType::kVideoPacket, 0);
  AV_LOGI(DEC_TAG, "[reddecoder] oh video decoder get delayed frame\n");
  return decode(&buffer);
}

VideoCodecError
HarmonyVideoDecoder::set_hardware_context(HardWareContext *context) {
  HarmonyHardWareContext *ctx =
      reinterpret_cast<HarmonyHardWareContext *>(context);
  if (!ctx || !ctx->natvie_window) {
    return VideoCodecError::kInvalidParameter;
  }
  if (native_window_) {
    OH_NativeWindow_NativeObjectUnreference(native_window_);
  }
  if (ctx->natvie_window) {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] %s acquire new native_window %p\n", this,
            __func__, ctx->natvie_window);
    OH_NativeWindow_NativeObjectReference(
        reinterpret_cast<OHNativeWindow *>(ctx->natvie_window));
  }
  native_window_ = reinterpret_cast<OHNativeWindow *>(ctx->natvie_window);

  init_video_decoder();
  return VideoCodecError::kNoError;
}

VideoCodecError
HarmonyVideoDecoder::update_hardware_context(HardWareContext *context) {
  HarmonyHardWareContext *ctx =
      reinterpret_cast<HarmonyHardWareContext *>(context);
  if (!ctx) {
    return VideoCodecError::kInvalidParameter;
  }

  release_video_decoder();

  if (native_window_) {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] %s release old native_window %p\n", this,
            __func__, native_window_);
    OH_NativeWindow_NativeObjectUnreference(native_window_);
  }

  if (ctx->natvie_window) {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] %s acquire new native_window %p\n", this,
            __func__, ctx->natvie_window);
    OH_NativeWindow_NativeObjectReference(
        reinterpret_cast<OHNativeWindow *>(ctx->natvie_window));
  }

  native_window_ = reinterpret_cast<OHNativeWindow *>(ctx->natvie_window);

  return VideoCodecError::kNoError;
}

int HarmonyVideoDecoder::get_serial() {
  int ret = serial_.load();
  return ret;
}

void HarmonyVideoDecoder::on_error_callback(int32_t error) {
  if (video_decoded_callback_) {
    video_decoded_callback_->on_decode_error(VideoCodecError::kInternalError,
                                             error);
  }
}

} // namespace reddecoder
