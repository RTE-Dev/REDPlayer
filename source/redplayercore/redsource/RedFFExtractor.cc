#include "RedFFExtractor.h"
#include "RedFFUtil.h"
#include "RedLog.h"
#include "RedMsg.h"
#include <iostream>
REDSOURCE_NS_BEGIN
RedFFExtractor::RedFFExtractor(const int &session_id, NotifyCallback notify_cb)
    : session_id_(session_id), notify_cb_(notify_cb) {
  ic_ = avformat_alloc_context();
  ic_->interrupt_callback.callback = interrupt_cb;
  ic_->interrupt_callback.opaque = static_cast<void *>(this);
}

RedFFExtractor::RedFFExtractor() : RedFFExtractor(0, nullptr) {}

int RedFFExtractor::seek(int64_t timestamp, int64_t rel, int seek_flags) {
  if (ic_->start_time != AV_NOPTS_VALUE) {
    timestamp += ic_->start_time;
  }
  int64_t seek_min = rel > 0 ? timestamp - rel + 2 : INT64_MIN;
  int64_t seek_max = rel < 0 ? timestamp - rel - 2 : INT64_MAX;
  int ret =
      avformat_seek_file(ic_, -1, seek_min, timestamp, seek_max, seek_flags);
  return ret;
}

static int meta_get_rotate(AVStream *st) {

  if (!st)
    return -1;

  int theta = std::abs(static_cast<int>(
      static_cast<int64_t>(round(fabs(redsource::get_rotation(st)))) % 360));

  switch (theta) {
  case 0:
  case 90:
  case 180:
  case 270:
    break;
  case 360:
    theta = 0;
    break;
  default:
    AV_LOGW(SOURCE_LOG_TAG, "[%s,%d]Unknown rotate degress: %d! \n",
            __FUNCTION__, __LINE__, theta);
    theta = 0;
    break;
  }

  return theta;
}

int RedFFExtractor::open(const std::string &url, FFMpegOpt &opt,
                         std::shared_ptr<MetaData> &metadata) {
  if (!ic_ || url.empty() || metadata == nullptr) {
    AV_LOGE_ID(SOURCE_LOG_TAG, session_id_,
               "[%s,%d][%d-%d-%d]extractor_ null! \n", __FUNCTION__, __LINE__,
               !ic_, url.empty(), metadata == nullptr);
    return -1;
  }
  int ret = 0;
  ret = avformat_open_input(&ic_, url.c_str(), nullptr,
                            opt.format_opts ? &opt.format_opts : nullptr);
  if (ret < 0) {
    AV_LOGE_ID(SOURCE_LOG_TAG, session_id_,
               "avformat_open_input failed!ret=%d\n", ret);
    return ret;
  }
  notifyListener(RED_MSG_OPEN_INPUT);
  av_format_inject_global_side_data(ic_);
  AVDictionary **opts = setup_find_stream_info_opts(ic_, opt.codec_opts);
  int orig_nb_streams = ic_->nb_streams;
  do {
    if (av_stristart(url.c_str(), "data:", NULL) && orig_nb_streams > 0) {
      int i = 0;
      for (i = 0; i < orig_nb_streams; i++) {
        if (!ic_->streams[i] || !ic_->streams[i]->codecpar ||
            ic_->streams[i]->codecpar->profile == FF_PROFILE_UNKNOWN) {
          break;
        }
      }

      if (i == orig_nb_streams) {
        break;
      }
    }
    ret = avformat_find_stream_info(ic_, opts);
  } while (0);

  notifyListener(RED_MSG_FIND_STREAM_INFO);

  for (int i = 0; i < orig_nb_streams; i++) {
    av_dict_free(&opts[i]);
  }
  av_freep(&opts);

  if (ret < 0) {
    AV_LOGE_ID(SOURCE_LOG_TAG, session_id_,
               "avformat_find_stream_info failed!ret=%d\n", ret);
    notifyListener(RED_MSG_ERROR, RED_MSG_FIND_STREAM_INFO, (int32_t)ret);
    return ret;
  }

  if (ic_->iformat && ic_->iformat->long_name) {
    metadata->format_type = ic_->iformat->long_name;
  }
  metadata->duration = ic_->duration;
  metadata->start_time = ic_->start_time;
  metadata->bit_rate = ic_->bit_rate;
  for (int i = 0; i < ic_->nb_streams; i++) {
    TrackInfo info;
    AVStream *st = ic_->streams[i];
    info.stream_index = st->index;
    info.rotation = meta_get_rotate(st);
    if (st->avg_frame_rate.num > 0 && st->avg_frame_rate.den > 0) {
      info.fps_num = st->avg_frame_rate.num;
      info.fps_den = st->avg_frame_rate.den;
    }
    if (st->r_frame_rate.num > 0 && st->r_frame_rate.den > 0) {
      info.tbr_num = st->r_frame_rate.num;
      info.tbr_den = st->r_frame_rate.den;
    }
    if (st->time_base.num > 0 && st->time_base.den > 0) {
      info.time_base_num = st->time_base.num;
      info.time_base_den = st->time_base.den;
    }
    if (st->codecpar) {
      info.stream_type = st->codecpar->codec_type;
      info.codec_id = st->codecpar->codec_id;
      info.codec_profile = st->codecpar->profile;
      info.codec_level = st->codecpar->level;
      int64_t bitrate = get_bit_rate(st->codecpar);
      info.bit_rate = bitrate;
      info.width = st->codecpar->width;
      info.height = st->codecpar->height;
      info.sar_num = st->codecpar->sample_aspect_ratio.num;
      info.sar_den = st->codecpar->sample_aspect_ratio.den;
      info.sample_rate = st->codecpar->sample_rate;
      info.sample_fmt = st->codecpar->format;
      info.channel_layout = st->codecpar->channel_layout;
      info.channels = st->codecpar->channels;
      info.extra_data_size = st->codecpar->extradata_size;
      info.color_primaries = st->codecpar->color_primaries;
      info.color_trc = st->codecpar->color_trc;
      info.color_space = st->codecpar->color_space;
      info.color_range = st->codecpar->color_range;
      if (st->codecpar->extradata_size > 0 && st->codecpar->extradata) {
        info.extra_data = new uint8_t[st->codecpar->extradata_size + 1];
        memcpy(info.extra_data, st->codecpar->extradata,
               st->codecpar->extradata_size);
      }
    }
    if (st->codec) {
      info.skip_loop_filter = st->codec->skip_loop_filter;
      info.skip_idct = st->codec->skip_idct;
      info.skip_frame = st->codec->skip_frame;
      info.pixel_format = st->codec->pix_fmt;
    }
    // st->discard = AVDISCARD_ALL;
    metadata->track_info.emplace_back(info);
  }

  return 0;
}

RedFFExtractor::~RedFFExtractor() {
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_,
             "[%s:%d] RedFFExtractor Deconstruct\n", __FUNCTION__, __LINE__);
}

void RedFFExtractor::close() {
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_,
             "[%s:%d] RedFFExtractor close start\n", __FUNCTION__, __LINE__);
  if (ic_) {
    avformat_close_input(&ic_);
    ic_ = nullptr;
  }
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_, "[%s:%d] RedFFExtractor close end\n",
             __FUNCTION__, __LINE__);
}

int RedFFExtractor::readPacket(AVPacket *pkt) {
  int ret = -1;
  if (ic_) {
    ret = av_read_frame(ic_, pkt);
  }
  return ret;
}

int RedFFExtractor::interrupt_cb(void *opaque) {
  return static_cast<RedFFExtractor *>(opaque)->interrupted_.load(
      std::memory_order_relaxed);
}

void RedFFExtractor::notifyListener(uint32_t what, int32_t arg1, int32_t arg2,
                                    void *obj1, void *obj2, int obj1_len,
                                    int obj2_len) {
  if (notify_cb_) {
    notify_cb_(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

void RedFFExtractor::setInterrupt() {
  AV_LOGD_ID(SOURCE_LOG_TAG, session_id_, "[%s:%d] interrupt.\n", __FUNCTION__,
             __LINE__);
  interrupted_.store(true);
}

int RedFFExtractor::getPbError() {
  int ret = 0;
  if (ic_ && ic_->pb) {
    ret = ic_->pb->error;
  }
  return ret;
}

int RedFFExtractor::getStreamType(int stream_index) {
  int ret = -1;
  if (ic_ && ic_->streams && stream_index < ic_->nb_streams) {
    ret = static_cast<int>(ic_->streams[stream_index]->codecpar->codec_type);
  }
  return ret;
}
REDSOURCE_NS_END
