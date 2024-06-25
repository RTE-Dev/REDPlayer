#include "reddecoder/audio/audio_decoder/ffmpeg_audio_decoder.h"
#include "reddecoder/common/logger.h"

namespace reddecoder {

static void release_av_frame(FFmpegBufferContext *context) {
  if (context && context->av_frame) {
    AVFrame *frame = reinterpret_cast<AVFrame *>(context->av_frame);
    av_frame_unref(frame);
    av_frame_free(&frame);
  }
}

FFmpegAudioDecoder::FFmpegAudioDecoder(AudioCodecInfo codec)
    : AudioDecoder(codec) {}

FFmpegAudioDecoder::~FFmpegAudioDecoder() { release(); }

AudioCodecError FFmpegAudioDecoder::init(AudioCodecConfig &config) {
  AudioCodecError err = release();
  if (err != AudioCodecError::kNoError) {
    return err;
  }
  if ((codec_info_.codec_name != AudioCodecName::kAAC &&
       codec_info_.codec_name != AudioCodecName::kOpaque) ||
      !config.channels || !config.sample_rate) {
    return AudioCodecError::kInvalidParameter;
  }
  codec_context_.reset(avcodec_alloc_context3(nullptr));

  codec_context_->codec_type = AVMEDIA_TYPE_AUDIO;
  codec_context_->thread_count = 1;
  codec_context_->time_base = {1, 1000};

  if (codec_info_.codec_name == AudioCodecName::kAAC) {
    codec_context_->codec_id = AV_CODEC_ID_AAC;
    if (codec_info_.profile == AudioCodecProfile::kHev1) {
      codec_context_->profile = FF_PROFILE_AAC_HE;
    } else if (codec_info_.profile == AudioCodecProfile::kHev2) {
      codec_context_->profile = FF_PROFILE_AAC_HE_V2;
    } else if (codec_info_.profile == AudioCodecProfile::kMain) {
      codec_context_->profile = FF_PROFILE_AAC_MAIN;
    }
  } else {
    codec_context_->codec_id = (AVCodecID)codec_info_.opaque_codec_id;
    codec_context_->profile = codec_info_.opaque_profile;
  }

  codec_context_->sample_rate = config.sample_rate;
  codec_context_->channels = config.channels;

  if (config.extradata_size > 0) {
    codec_context_->extradata = reinterpret_cast<uint8_t *>(
        av_malloc(config.extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
    codec_context_->extradata_size = config.extradata_size;
    memcpy(codec_context_->extradata, config.extradata, config.extradata_size);
  }

  AVCodec *codec = avcodec_find_decoder(codec_context_->codec_id);
  if (!codec) {
    release();
    return AudioCodecError::kInitError;
  }

  AVDictionary *opts = NULL;
  int ret = 0;
  av_dict_set(&opts, "refcounted_frames", "1", 0);
  if ((ret = avcodec_open2(codec_context_.get(), codec, nullptr)) < 0) {
    av_dict_free(&opts);
    release();
    return AudioCodecError::kInitError;
  }
  av_dict_free(&opts);

  return AudioCodecError::kNoError;
}

AudioCodecError FFmpegAudioDecoder::release() {
  codec_context_.reset();
  return AudioCodecError::kNoError;
}

AudioCodecError FFmpegAudioDecoder::decode(const Buffer *buffer) {
  if (!codec_context_ || !audio_decoded_callback_) {
    return AudioCodecError::kUninited;
  }

  if (!buffer || buffer->get_type() != BufferType::kAudioPacket) {
    return AudioCodecError::kInvalidParameter;
  }

  AVPacket packet;
  av_init_packet(&packet);
  packet.data = buffer->get_data();
  packet.size = buffer->get_size();
  packet.pts = buffer->get_audio_packet_meta()->pts_ms;

  if (avcodec_send_packet(codec_context_.get(), &packet) < 0) {
    return AudioCodecError::kInternalError;
  }

  AVFrame *frame = av_frame_alloc();
  int ret = avcodec_receive_frame(codec_context_.get(), frame);
  if (ret < 0) {
    av_frame_free(&frame);
    switch (ret) {
    case AVERROR(EAGAIN): {
      return AudioCodecError::kNoError;
    }
    case AVERROR_EOF: {
      return AudioCodecError::kEOF;
    }
    default:
      return AudioCodecError::kInternalError;
    }
  }

  size_t buffer_size = 1;
  std::unique_ptr<Buffer> output_buffer =
      std::make_unique<Buffer>(BufferType::kAudioFrame, buffer_size);

  AudioFrameMeta *meta = output_buffer->get_audio_frame_meta();

  for (int i = 0; i < frame->channels; i++) {
    meta->channel[i] = frame->data[i];
  }

  meta->buffer_context = reinterpret_cast<void *>(new FFmpegBufferContext{
      .av_frame = frame,
      .release_av_frame = release_av_frame,
  });
  meta->num_channels = frame->channels;
  meta->sample_rate = frame->sample_rate;
  meta->num_samples = frame->nb_samples;
  meta->sample_format = frame->format;
  meta->channel_layout = frame->channel_layout;
  meta->pts_ms = frame->best_effort_timestamp;

  audio_decoded_callback_->on_decoded_frame(std::move(output_buffer));

  return AudioCodecError::kNoError;
}

AudioCodecError FFmpegAudioDecoder::flush() {
  if (codec_context_) {
    avcodec_flush_buffers(codec_context_.get());
  }
  return AudioCodecError::kNoError;
}

} // namespace reddecoder
