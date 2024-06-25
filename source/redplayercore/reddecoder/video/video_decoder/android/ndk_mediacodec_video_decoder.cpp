#include "reddecoder/video/video_decoder/android/ndk_mediacodec_video_decoder.h"

#include <android/log.h>
#include <android/native_window.h>

#include <memory>

#include "media/NdkMediaExtractor.h"
#include "reddecoder/common/logger.h"
#include "reddecoder/video/video_common/format_convert_helper.h"

extern "C" {
#include "libavutil/avutil.h"
}

#define AMC_INPUT_TIMEOUT_US (30 * 1000)
#define AMC_OUTPUT_TIMEOUT_US (10 * 1000)

namespace reddecoder {

static void mediacodec_buffer_release(MediaCodecBufferContext *context,
                                      bool render) {
  if (context && context->media_codec && context->decoder) {
    if (context->decoder_serial ==
        (reinterpret_cast<Ndk_MediaCodecVideoDecoder *>(context->decoder))
            ->get_serial()) {
      AMediaCodec_releaseOutputBuffer(
          reinterpret_cast<AMediaCodec *>(context->media_codec),
          context->buffer_index, render);
    } else {
      AV_LOGW(DEC_TAG,
              "[reddecoder] release buffer serial is not equal, index:%d "
              "serial:%d\n",
              context->buffer_index, context->decoder_serial);
    }
  }
}

Ndk_MediaCodecVideoDecoder::Ndk_MediaCodecVideoDecoder(VideoCodecInfo codec)
    : VideoDecoder(codec) {
  std::atomic_init(&serial_, 1);
}
Ndk_MediaCodecVideoDecoder::~Ndk_MediaCodecVideoDecoder() { release(); }

VideoCodecError Ndk_MediaCodecVideoDecoder::init(const Buffer *buffer) {
  if ((codec_info_.codec_name != VideoCodecName::kH264 &&
       codec_info_.codec_name != VideoCodecName::kH265) ||
      codec_info_.implementation_type !=
          VideoCodecImplementationType::kHardwareAndroid) {
    return VideoCodecError::kInitError;
  }
  release();

  if (buffer) {
    set_hardware_context(
        buffer->get_video_format_desc_meta()->hardware_context);
    if (buffer->get_size() > 0) {
      init_media_format_desc(buffer);
      init_media_codec();
    }
  }
  AV_LOGI(DEC_TAG, "[reddecoder_%p] init \n", this);
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::release() {
  release_media_codec();
  release_media_format_desc();
  AV_LOGI(DEC_TAG, "[reddecoder_%p] release \n", this);
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::feed_decoder(const Buffer *buffer,
                                                         size_t &offset) {
  VideoPacketMeta *meta = buffer->get_video_packet_meta();
  while ((offset < buffer->get_size() || buffer->get_size() == 0) &&
         !is_drain_state_) {
    ssize_t buf_idx = current_input_buffer_;
    if (buf_idx < 0) {
      buf_idx = AMediaCodec_dequeueInputBuffer(media_codec_.get(),
                                               AMC_INPUT_TIMEOUT_US);
      if (buf_idx < 0) {
        if (buf_idx == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
          return VideoCodecError::kTryAgain;
        }
        AV_LOGE(DEC_TAG,
                "[reddecoder] mediacodec dequeue input buffer error: %zd\n",
                buf_idx);
        return VideoCodecError::kInternalError;
      }
    }
    current_input_buffer_ = -1;

    size_t buf_size;
    uint8_t *buf =
        AMediaCodec_getInputBuffer(media_codec_.get(), buf_idx, &buf_size);
    if (buf_size <= 0 || buf == NULL) {
      AV_LOGE(DEC_TAG, "[reddecoder] mediacodec get input buffer error: %d\n",
              static_cast<int>(buf_size));
      return VideoCodecError::kInternalError;
    }

    size_t copy_size = (buffer->get_size() - offset) < buf_size
                           ? (buffer->get_size() - offset)
                           : buf_size;

    if (buffer->get_size() == 0) {
      is_drain_state_ = true;
      AV_LOGI(DEC_TAG,
              "[reddecoder] mediacodec input end of stream copy_size:%zu, "
              "buffer_size:%zu, buffer_size:%zu, offset:%zu\n",
              copy_size, buffer->get_size(), buf_size, offset);
    }

    if (copy_size > 0 && copy_size <= buf_size &&
        (offset + copy_size <= buffer->get_size())) {
      memcpy(buf, buffer->get_data() + offset,
             copy_size); // when offset > 0, what pts to put in
      offset += copy_size;
    } else {
      if (buffer->get_size() != 0) { // not last packet
        AV_LOGE(DEC_TAG,
                "[reddecoder] error during memcpy, copy_size: %zu, "
                "buffer_size:%zu, buffer_size:%zu, offset:%zu\n",
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

    auto status = AMediaCodec_queueInputBuffer(
        media_codec_.get(), buf_idx, 0, copy_size, pts,
        is_drain_state_ ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
    if (status != AMEDIA_OK) {
      AV_LOGE(DEC_TAG, "[reddecoder] mediacodec queue input buffer error: %d\n",
              static_cast<int>(status));
      return VideoCodecError::kInternalError;
    }
  }
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::drain_decoder() {
  if (!is_eof_state_) {
    AMediaCodecBufferInfo info;
    int64_t drain_timeout_us = AMC_OUTPUT_TIMEOUT_US;
    if (!output_first_frame_state_) {
      drain_timeout_us = 0;
    }
    auto status = AMediaCodec_dequeueOutputBuffer(media_codec_.get(), &info,
                                                  drain_timeout_us);
    if (status >= 0) {
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
      meta->pixel_format = PixelFormat::kMediaCodecBuffer;
      meta->pts_ms = info.presentationTimeUs / 1000;
      if (meta->pts_ms < 0) {
        meta->pts_ms = AV_NOPTS_VALUE;
      }
      meta->buffer_context =
          reinterpret_cast<void *>(new MediaCodecBufferContext{
              .buffer_index = static_cast<int>(status),
              .media_codec = media_codec_.get(),
              .decoder_serial = serial_.load(),
              .decoder = this,
              .release_output_buffer = mediacodec_buffer_release,
          });

      if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
        AV_LOGI(DEC_TAG, "[reddecoder] mediacodec output end of stream\n");
        is_eof_state_ = true;
        return VideoCodecError::kEOF;
      }

      if (!output_first_frame_state_) {
        output_first_frame_state_ = true;
      }

      video_decoded_callback_->on_decoded_frame(std::move(buffer));

    } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
      AV_LOGW(DEC_TAG, "[reddecoder] mediacodec output buffer changed\n");
    } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
      auto format = AMediaCodec_getOutputFormat(media_codec_.get());
      AV_LOGI(DEC_TAG, "[reddecoder] mediacodec output format changed to %s\n",
              AMediaFormat_toString(format));
      AMediaFormat_delete(format);
    } else {
      AV_LOGE(DEC_TAG, "[reddecoder] mediacodec unexpected info code: %zd\n",
              status);
      if (is_drain_state_) {
        AV_LOGE(
            DEC_TAG,
            "[reddecoder] mediacodec return eof due to drain process error");
        return VideoCodecError::kEOF;
      }
    }
  } else {
    return VideoCodecError::kEOF;
  }
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::decode(const Buffer *buffer) {
  if (!buffer || buffer->get_type() != BufferType::kVideoPacket) {
    return VideoCodecError::kInvalidParameter;
  }

  if (buffer->get_video_packet_meta()->format == VideoPacketFormat::kAVCC ||
      (buffer->get_video_packet_meta()->format ==
           VideoPacketFormat::kFollowExtradataFormat &&
       !codec_ctx_.is_annexb_extradata)) {
    FormatConvertHelper::H2645ConvertState convert_state = {0, 0};
    FormatConvertHelper::convert_h2645_to_annexb(
        buffer->get_data(), buffer->get_size(), codec_ctx_.nal_size,
        &convert_state);
  }

  if (!media_format_ || !media_codec_) {
    return VideoCodecError::kInitError;
  }

  size_t offset = 0;
  if (buffer->get_video_packet_meta()->offset > 0) {
    offset = buffer->get_video_packet_meta()->offset;
  }

  int retry_threshold = 15;
  int retry_time = 0;

  while (1) {
    VideoCodecError ret = drain_decoder();
    if (ret == VideoCodecError::kEOF) {
      return ret;
    }

    ret = feed_decoder(buffer, offset);
    if (ret == VideoCodecError::kTryAgain) {
      if (++retry_time > retry_threshold) {
        AV_LOGE(DEC_TAG, "[reddecoder] mediacodec timeout reach threshold\n");
        buffer->get_video_packet_meta()->offset = offset;
        return ret;
      }
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
Ndk_MediaCodecVideoDecoder::init_media_format_desc(const Buffer *buffer) {
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
  media_format_.reset(AMediaFormat_new());

  size_t sps_pps_size = 0;
  size_t convert_size = extradata_size + 20;
  uint8_t *convert_buffer =
      reinterpret_cast<uint8_t *>(calloc(1, convert_size));
  if (!convert_buffer) {
    return VideoCodecError::kInitError;
  }

  if (extradata[0] == 1 || extradata[1] == 1) { // avcc format
    size_t nal_size = 0;
    if (codec_info_.codec_name == VideoCodecName::kH264) {
      if (0 != FormatConvertHelper::convert_sps_pps(
                   extradata, extradata_size, convert_buffer, convert_size,
                   &sps_pps_size, &nal_size)) {
        return VideoCodecError::kInitError;
      }
    } else if (codec_info_.codec_name == VideoCodecName::kH265) {
      if (0 != FormatConvertHelper::convert_hevc_nal_units(
                   extradata, extradata_size, convert_buffer, convert_size,
                   &sps_pps_size, &nal_size)) {
        return VideoCodecError::kInitError;
      }
    }
    codec_ctx_.nal_size = nal_size;
    AMediaFormat_setBuffer(media_format_.get(), "csd-0", convert_buffer,
                           sps_pps_size);
    AV_LOGI(DEC_TAG,
            "[reddecoder_%p] reset video format, extra data size %zu\n", this,
            extradata_size);
  } else {
    codec_ctx_.is_annexb_extradata = true;
  }

  if (codec_info_.codec_name == VideoCodecName::kH264) {
    AMediaFormat_setString(media_format_.get(), AMEDIAFORMAT_KEY_MIME,
                           "video/avc");
  } else if (codec_info_.codec_name == VideoCodecName::kH265) {
    AMediaFormat_setString(media_format_.get(), AMEDIAFORMAT_KEY_MIME,
                           "video/hevc");
  }

  codec_ctx_.width = meta->width;
  codec_ctx_.height = meta->height;
  codec_ctx_.rotate_degree = meta->rotate_degree;

  if (codec_ctx_.rotate_degree != 0) {
    AMediaFormat_setInt32(media_format_.get(), "rotation-degrees",
                          codec_ctx_.rotate_degree);
  }

  AMediaFormat_setInt32(media_format_.get(), AMEDIAFORMAT_KEY_WIDTH,
                        codec_ctx_.width);
  AMediaFormat_setInt32(media_format_.get(), AMEDIAFORMAT_KEY_HEIGHT,
                        codec_ctx_.height);
  AMediaFormat_setInt32(media_format_.get(), AMEDIAFORMAT_KEY_MAX_INPUT_SIZE,
                        0);
  free(convert_buffer);
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::release_media_format_desc() {
  media_format_.reset();
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::init_media_codec() {
  release_media_codec();

  if (!media_format_) {
    return VideoCodecError::kUninited;
  }
  current_input_buffer_ = -1;

  if (codec_info_.codec_name == VideoCodecName::kH264) {
    media_codec_.reset(AMediaCodec_createDecoderByType("video/avc"));
  } else if (codec_info_.codec_name == VideoCodecName::kH265) {
    media_codec_.reset(AMediaCodec_createDecoderByType("video/hevc"));
  }

  if (!media_codec_) {
    AV_LOGE(DEC_TAG, "[reddecoder_%p] mediacodec init error\n", this);
    return VideoCodecError::kInitError;
  } else {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] mediacodec inited %p\n", this,
            media_codec_.get());
  }

  media_status_t status = AMediaCodec_configure(
      media_codec_.get(), media_format_.get(), native_window_, nullptr, 0);

  if (status != AMEDIA_OK) {
    return VideoCodecError::kInitError;
  }

  status = AMediaCodec_start(media_codec_.get());

  if (status != AMEDIA_OK) {
    return VideoCodecError::kInitError;
  }

  is_mediacodec_start_ = true;

  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::release_media_codec() {
  serial_++;
  if (media_codec_ && is_mediacodec_start_) {
    media_status_t ret = AMediaCodec_stop(media_codec_.get());
    if (ret < 0) {
      AV_LOGE(DEC_TAG, "[reddecoder] media codec stop error %d\n",
              static_cast<int>(ret));
    }
  }
  media_codec_.reset();
  is_mediacodec_start_ = false;
  return VideoCodecError::kNoError;
}

VideoCodecError
Ndk_MediaCodecVideoDecoder::set_video_format_description(const Buffer *buffer) {
  if (buffer->get_type() != BufferType::kVideoFormatDesc ||
      !buffer->get_video_format_desc_meta()) {
    return VideoCodecError::kInvalidParameter;
  }

  AndroidHardWareContext *ctx = reinterpret_cast<AndroidHardWareContext *>(
      buffer->get_video_format_desc_meta()->hardware_context);
  if (ctx) {
    native_window_ = reinterpret_cast<ANativeWindow *>(ctx->natvie_window);
    AV_LOGI(DEC_TAG, "[reddecoder] set mediacodec native window %p\n",
            native_window_);
  }

  for (auto &item : buffer->get_video_format_desc_meta()->items) {
    if (item.type == VideoFormatDescType::kExtraData) {
      auto ret = init_media_format_desc(buffer);
      if (ret != VideoCodecError::kNoError) {
        return ret;
      }
      ret = init_media_codec();
      if (ret != VideoCodecError::kNoError) {
        return ret;
      }
    }
  }
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::flush() {
  serial_++;
  is_drain_state_ = false;
  is_eof_state_ = false;
  output_first_frame_state_ = false;
  if (media_codec_) {
    auto status = AMediaCodec_flush(media_codec_.get());
    if (status < 0) {
      AV_LOGE(DEC_TAG, "[reddecoder] mediacodec flush error: %d\n", status);
      return VideoCodecError::kInternalError;
    }
  }
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::get_delayed_frames() {
  Buffer buffer(BufferType::kVideoPacket, 0);
  AV_LOGI(DEC_TAG, "[reddecoder] mediacodec get delayed frames\n");
  while (decode(&buffer) == VideoCodecError::kNoError) {
  }
  is_drain_state_ = false;
  is_eof_state_ = false;
  output_first_frame_state_ = false;
  return VideoCodecError::kNoError;
}

VideoCodecError Ndk_MediaCodecVideoDecoder::get_delayed_frame() {
  Buffer buffer(BufferType::kVideoPacket, 0);
  AV_LOGI(DEC_TAG, "[reddecoder] mediacodec get delayed frame\n");
  return decode(&buffer);
}

VideoCodecError
Ndk_MediaCodecVideoDecoder::set_hardware_context(HardWareContext *context) {
  AndroidHardWareContext *ctx =
      reinterpret_cast<AndroidHardWareContext *>(context);
  if (!ctx || !ctx->natvie_window) {
    return VideoCodecError::kInvalidParameter;
  }
  if (native_window_) {
    ANativeWindow_release(native_window_);
  }
  ANativeWindow_acquire(reinterpret_cast<ANativeWindow *>(ctx->natvie_window));
  native_window_ = reinterpret_cast<ANativeWindow *>(ctx->natvie_window);

  init_media_codec();
  return VideoCodecError::kNoError;
}

VideoCodecError
Ndk_MediaCodecVideoDecoder::update_hardware_context(HardWareContext *context) {
  AndroidHardWareContext *ctx =
      reinterpret_cast<AndroidHardWareContext *>(context);
  if (!ctx) {
    return VideoCodecError::kInvalidParameter;
  }

  release_media_codec();

  if (native_window_) {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] %s release old native_window %p\n", this,
            __func__, native_window_);
    ANativeWindow_release(native_window_);
  }

  if (ctx->natvie_window) {
    AV_LOGI(DEC_TAG, "[reddecoder_%p] %s acquire new native_window %p\n", this,
            __func__, ctx->natvie_window);
    ANativeWindow_acquire(
        reinterpret_cast<ANativeWindow *>(ctx->natvie_window));
  }

  native_window_ = reinterpret_cast<ANativeWindow *>(ctx->natvie_window);

  return VideoCodecError::kNoError;
}

int Ndk_MediaCodecVideoDecoder::get_serial() {
  int ret = serial_.load();
  return ret;
}

} // namespace reddecoder
