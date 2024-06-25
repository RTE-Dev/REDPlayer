#include "reddecoder/video/video_decoder/ffmpeg_video_decoder.h"

#include "reddecoder/common/logger.h"

namespace reddecoder {

static void release_av_frame(FFmpegBufferContext *context) {
  if (context && context->av_frame) {
    AVFrame *frame = reinterpret_cast<AVFrame *>(context->av_frame);
    av_frame_unref(frame);
    av_frame_free(&frame);
  }
}

FFmpegVideoDecoder::FFmpegVideoDecoder(VideoCodecInfo codec)
    : VideoDecoder(codec) {}

FFmpegVideoDecoder::~FFmpegVideoDecoder() { release(); }

VideoCodecError FFmpegVideoDecoder::init(const Buffer *buffer) {
  VideoCodecError err = release();
  if (err != VideoCodecError::kNoError) {
    return err;
  }

  if (codec_info_.codec_name != VideoCodecName::kH264 &&
      codec_info_.codec_name != VideoCodecName::kH265 &&
      codec_info_.codec_name != VideoCodecName::kAV1 &&
      codec_info_.codec_name != VideoCodecName::kFFMPEGID) {
    return VideoCodecError::kInvalidParameter;
  }

  codec_context_.reset(avcodec_alloc_context3(nullptr));

  codec_context_->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context_->thread_count = 1;
  codec_context_->thread_type = FF_THREAD_SLICE;
  codec_context_->time_base = {1, 1000};

  if (codec_info_.codec_name == VideoCodecName::kH264) {
    codec_context_->codec_id = AV_CODEC_ID_H264;
  } else if (codec_info_.codec_name == VideoCodecName::kH265) {
    codec_context_->codec_id = AV_CODEC_ID_H265;
  } else if (codec_info_.codec_name == VideoCodecName::kAV1) {
    codec_context_->codec_id = AV_CODEC_ID_AV1;
  } else if (codec_info_.codec_name == VideoCodecName::kFFMPEGID) {
    codec_context_->codec_id = (AVCodecID)codec_info_.ffmpeg_codec_id;
  }

  AVCodec *codec;
  AV_LOGI(DEC_TAG, "[reddecoder] %s, use ffmpeg decoder %d\n", __FUNCTION__,
          codec_context_->codec_id);
  codec = avcodec_find_decoder(codec_context_->codec_id);

  if (!codec) {
    AV_LOGE(DEC_TAG, "[reddecoder] %s, codec init is nullptr\n", __FUNCTION__);
    release();
    return VideoCodecError::kInitError;
  }

  AVDictionary *opts = NULL;
  if (avcodec_open2(codec_context_.get(), codec, &opts) < 0) {
    release();
    av_dict_free(&opts);
    return VideoCodecError::kInitError;
  }
  av_dict_free(&opts);

  if (buffer) {
    return set_video_format_description(buffer);
  }

  return VideoCodecError::kNoError;
}

VideoCodecError FFmpegVideoDecoder::release() {
  codec_context_.reset();
  return VideoCodecError::kNoError;
}

VideoCodecError FFmpegVideoDecoder::flush() {
  is_drain_state_ = false;
  if (codec_context_) {
    avcodec_flush_buffers(codec_context_.get());
  }
  return VideoCodecError::kNoError;
}

VideoCodecError FFmpegVideoDecoder::decode(const Buffer *buffer) {
  if (!codec_context_ || !video_decoded_callback_) {
    return VideoCodecError::kUninited;
  }

  if (!buffer || buffer->get_type() != BufferType::kVideoPacket) {
    return VideoCodecError::kInvalidParameter;
  }

  AVFrame *frame = av_frame_alloc();
  int ret = avcodec_receive_frame(codec_context_.get(), frame);

  if (ret >= 0) {
    size_t buffer_size = 1;
    std::unique_ptr<Buffer> output_buffer =
        std::make_unique<Buffer>(BufferType::kVideoFrame, buffer_size);

    VideoFrameMeta *meta = output_buffer->get_video_frame_meta();
    meta->width = frame->width;
    meta->height = frame->height;
    meta->yStride = frame->linesize[0];
    meta->uStride = frame->linesize[1];
    meta->vStride = frame->linesize[2];

    meta->yBuffer = frame->data[0];
    meta->uBuffer = frame->data[1];
    meta->vBuffer = frame->data[2];

    meta->buffer_context = reinterpret_cast<void *>(new FFmpegBufferContext{
        .av_frame = frame,
        .release_av_frame = release_av_frame,
    });

    switch (frame->format) {
    case AV_PIX_FMT_YUVJ420P:
      meta->pixel_format = PixelFormat::kYUVJ420P;
      break;
    case AV_PIX_FMT_YUV420P10LE:
      meta->pixel_format = PixelFormat::kYUV420P10LE;
      break;
    case AV_PIX_FMT_YUV420P:
      meta->pixel_format = PixelFormat::kYUV420;
      break;
    default:
      meta->pixel_format = PixelFormat::kYUV420;
      AV_LOGW(DEC_TAG, "[reddecoder] %s, unexpected frame format %d\n",
              __FUNCTION__, frame->format);
      break;
    }

    meta->pts_ms = frame->best_effort_timestamp;
    meta->dts_ms = frame->pkt_dts;

    video_decoded_callback_->on_decoded_frame(std::move(output_buffer));
  } else if (ret == AVERROR_EOF) {
    av_frame_unref(frame);
    av_frame_free(&frame);
    return VideoCodecError::kEOF;
  } else {
    av_frame_unref(frame);
    av_frame_free(&frame);
  }

  AVPacket packet;
  av_init_packet(&packet);
  packet.data = buffer->get_data();
  packet.size = buffer->get_size();
  packet.pts = buffer->get_video_packet_meta()->pts_ms;
  packet.dts = buffer->get_video_packet_meta()->dts_ms;

  if (!is_drain_state_) {
    if (packet.size == 0) {
      is_drain_state_ = true;
    }
    if (avcodec_send_packet(codec_context_.get(),
                            is_drain_state_ ? NULL : &packet) ==
        AVERROR(EAGAIN)) {
      return VideoCodecError::kTryAgain;
    }
  }

  return VideoCodecError::kNoError;
}

AVDiscard FFmpegVideoDecoder::transfer_discard_opt_to_ffmpeg(VideoDiscard opt) {
  switch (opt) {
  case VideoDiscard::kDISCARD_NONE:
    return AVDISCARD_NONE;
  case VideoDiscard::kDISCARD_DEFAULT:
    return AVDISCARD_DEFAULT;
  case VideoDiscard::kVDISCARD_NONREF:
    return AVDISCARD_NONREF;
  case VideoDiscard::kVDISCARD_BIDIR:
    return AVDISCARD_BIDIR;
  case VideoDiscard::kVDISCARD_NONINTRA:
    return AVDISCARD_NONINTRA;
  case VideoDiscard::kVDISCARD_NONKEY:
    return AVDISCARD_NONKEY;
  case VideoDiscard::kVDISCARD_ALL:
    return AVDISCARD_ALL;
  default:
    return AVDISCARD_NONE;
  }
}

VideoCodecError
FFmpegVideoDecoder::set_video_format_description(const Buffer *buffer) {
  if (!buffer || buffer->get_type() != BufferType::kVideoFormatDesc ||
      !buffer->get_video_format_desc_meta()) {
    AV_LOGI(DEC_TAG, "[reddecoder] %s, buffer has problem\n", __FUNCTION__);
    return VideoCodecError::kInvalidParameter;
  }

  if (codec_context_ == nullptr) {
    AV_LOGI(DEC_TAG, "[reddecoder] %s, codec_context_ is nullptr\n",
            __FUNCTION__);
    return VideoCodecError::kInvalidParameter;
  }

  for (auto &item : buffer->get_video_format_desc_meta()->items) {
    if (item.type == VideoFormatDescType::kExtraData) {
      AV_LOGI(DEC_TAG, "[reddecoder] %s, extradataSize is %zu\n", __FUNCTION__,
              buffer->get_size());
      codec_context_->extradata = reinterpret_cast<uint8_t *>(
          av_malloc(buffer->get_size() + AV_INPUT_BUFFER_PADDING_SIZE));
      codec_context_->extradata_size = buffer->get_size();
      memcpy(codec_context_->extradata, buffer->get_data(), buffer->get_size());

      VideoFormatDescMeta *meta = buffer->get_video_format_desc_meta();
      if (!meta) {
        return VideoCodecError::kInvalidParameter;
      }

      codec_context_->skip_frame =
          transfer_discard_opt_to_ffmpeg(meta->skip_frame);
      codec_context_->skip_loop_filter =
          transfer_discard_opt_to_ffmpeg(meta->skip_loop_filter);
      codec_context_->skip_idct =
          transfer_discard_opt_to_ffmpeg(meta->skip_idct);
      AV_LOGI(DEC_TAG,
              "[reddecoder] %s, ffmpeg discard opt skip_frame:%d skip_loop:%d "
              "skip_idct%d\n",
              __FUNCTION__, codec_context_->skip_frame,
              codec_context_->skip_loop_filter, codec_context_->skip_idct);

      AVCodec *codec;
      codec = avcodec_find_decoder(codec_context_->codec_id);
      if (!codec) {
        release();
        return VideoCodecError::kInitError;
      }

      int ret = 0;
      if ((ret = avcodec_close(codec_context_.get())) < 0) {
        release();
        return VideoCodecError::kInitError;
      }

      AVDictionary *opts = NULL;
      av_dict_set(&opts, "threads", "auto", 0);
      av_dict_set(&opts, "refcounted_frames", "1", 0);

      if ((ret = avcodec_open2(codec_context_.get(), codec, &opts)) < 0) {
        av_dict_free(&opts);
        release();
        return VideoCodecError::kInitError;
      }
      av_dict_free(&opts);
    }
  }
  return VideoCodecError::kNoError;
}

VideoCodecError FFmpegVideoDecoder::get_delayed_frames() {
  Buffer buffer(BufferType::kVideoPacket, 0);
  while (decode(&buffer) != VideoCodecError::kEOF) {
  }
  is_drain_state_ = false;
  return VideoCodecError::kNoError;
}

VideoCodecError FFmpegVideoDecoder::get_delayed_frame() {
  Buffer buffer(BufferType::kVideoPacket, 0);
  return decode(&buffer);
}

VideoCodecError FFmpegVideoDecoder::set_skip_frame(int skip_frame) {
  if (!codec_context_) {
    return VideoCodecError::kInitError;
  }
  codec_context_->skip_frame =
      (AVDiscard)std::min(skip_frame, static_cast<int>(AVDISCARD_ALL));
  return VideoCodecError::kNoError;
}

} // namespace reddecoder
