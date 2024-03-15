#pragma once

#include "RedBase.h"
#include "RedDef.h"
#include "RedError.h"

#include "RedBuffer.h"
#include "RedClock.h"
#include "RedPacket.h"
#include "RedSampler.h"

#include <iostream>
#include <stdint.h>
#include <unistd.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "reddecoder/video/video_decoder/video_decoder_factory.h"
#include <atomic>
#include <chrono>

REDPLAYER_NS_BEGIN;

enum {
  TYPE_UNKNOWN = -1,
  TYPE_VIDEO = 0,
  TYPE_AUDIO,
  TYPE_DATA,
  TYPE_SUBTITLE
};

class CRedThreadBase {
public:
  CRedThreadBase();
  virtual ~CRedThreadBase();

  virtual void ThreadFunc() = 0;

  void run();

protected:
  bool mAbort{false};
  int mSerial{0};
  std::thread mThread;
};

class PktQueue {
public:
  PktQueue() = default;
  explicit PktQueue(int type);
  ~PktQueue() = default;
  RED_ERR putPkt(std::unique_ptr<RedAvPacket> &pkt);
  RED_ERR getPkt(std::unique_ptr<RedAvPacket> &pkt, bool block);
  bool frontIsFlush();
  void flush();
  void abort();
  void clear();
  size_t size();
  int64_t bytes();
  int64_t duration();

private:
  std::mutex mLock;
  std::condition_variable mNotEmptyCond;
  std::queue<std::unique_ptr<RedAvPacket>> mPktQueue;
  int64_t mBytes{0};
  int64_t mDuration{0};
  bool mAbort{false};
};

class FrameQueue {
public:
  FrameQueue() = default;
  FrameQueue(size_t capacity, int type);
  ~FrameQueue() = default;
  RED_ERR putFrame(std::unique_ptr<CGlobalBuffer> &frame);
  RED_ERR getFrame(std::unique_ptr<CGlobalBuffer> &frame);
  void flush();
  void abort();
  void wakeup();
  size_t size();

private:
  std::mutex mLock;
  std::condition_variable mNotEmptyCond;
  std::condition_variable mNotFullCond;
  std::queue<std::unique_ptr<CGlobalBuffer>> mFrameQueue;
  size_t mCapacity{FRAME_QUEUE_SIZE};
  bool mAbort{false};
  bool mWakeup{false};
};

typedef struct FFTrackCacheStatistic {
  int64_t duration{0};
  int64_t bytes{0};
  int64_t packets{0};
} FFTrackCacheStatistic;

typedef struct FFStatistic {
  int64_t vdec_type{0};

  float vfps{0.0};
  float vdps{0.0};
  float avdelay{0.0};
  float avdiff{0.0};
  int64_t bit_rate{0};
  int pixel_format{0};
  int drop_packet_count{0};
  int total_packet_count{0};

  FFTrackCacheStatistic video_cache;
  FFTrackCacheStatistic audio_cache;

  SpeedSampler2 tcp_read_sampler;
  int64_t latest_seek_load_duration{0};
  int64_t byte_count{0};
  int64_t byte_count_inet{0};  // ipv4  下载量
  int64_t byte_count_inet6{0}; // ipv6  下载量
  bool isaf_inet6{false};      // 是否ipv6 协议
  int64_t cache_physical_pos{0};
  int64_t cache_file_forwards{0};
  int64_t cache_file_pos{0};
  int64_t cache_count_bytes{0};
  int64_t logical_file_size{0};
  int64_t cached_size{0};
  int64_t last_cached_pos{0};
  int drop_frame_count{0};
  int decode_frame_count{0};
  int refresh_decoder_count{0};
  float drop_frame_rate{0.0};
  int64_t real_cached_size{-1};
} FFStatistic;

struct VideoState {
  FFStatistic stat;

  int frame_drops_early{0};
  int continuous_frame_drops_early{0};

  volatile int latest_video_seek_load_serial{-1};
  volatile int latest_audio_seek_load_serial{-1};
  volatile int64_t latest_seek_load_start_at{0};

  int64_t current_position_ms{0};
  int64_t playable_duration_ms{0};
  bool backup_url_is_null{0};
  bool step_to_next_frame{false};

  bool auddec_finished{false};
  bool viddec_finished{false};

  bool pause_req{false};
  bool paused{false};

  int loop_count{0};
  int first_video_frame_rendered{0};
  int first_audio_frame_rendered{0};

  int audio_stream_index{-1};
  int video_stream_index{-1};
  int av_sync_type{CLOCK_EXTER};

  float playback_rate{1.0f};
  float volume{1.0f};
  bool is_video_high_fps{false};
  int nal_length_size{0};
  int skip_frame{DISCARD_DEFAULT};
  bool seek_req{false};
  int64_t seek_pos{0};
  int64_t seek_rel{0};
  int drop_aframe_count{0};
  int drop_vframe_count{0};
  int64_t accurate_seek_start_time{0};
  volatile int64_t accurate_seek_vframe_pts{0};
  volatile int64_t accurate_seek_aframe_pts{0};
  bool audio_accurate_seek_req{false};
  bool video_accurate_seek_req{false};
  std::mutex accurate_seek_mutex;
  std::condition_variable video_accurate_seek_cond;
  std::condition_variable audio_accurate_seek_cond;
  std::unique_ptr<RedClock> audio_clock;
  std::unique_ptr<RedClock> video_clock;
  std::unique_ptr<RedClock> external_clock;
  std::string video_codec_info;
  std::string audio_codec_info;
  std::string play_url;
  int error{0};
};
static inline int getMasterSyncType(const sp<VideoState> is) {
  if (!is) {
    return CLOCK_EXTER;
  }
  if (is->av_sync_type == CLOCK_VIDEO) {
    if (is->video_stream_index >= 0) {
      return CLOCK_VIDEO;
    } else {
      return CLOCK_AUDIO;
    }
  } else if (is->av_sync_type == CLOCK_AUDIO) {
    if (is->audio_stream_index >= 0) {
      return CLOCK_AUDIO;
    } else {
      return CLOCK_EXTER;
    }
  } else {
    return CLOCK_EXTER;
  }
}

static inline double getMasterClock(const sp<VideoState> is) {
  double val = NAN;

  if (!is) {
    return val;
  }

  switch (getMasterSyncType(is)) {
  case CLOCK_VIDEO:
    val = is->video_clock->GetClock();
    break;
  case CLOCK_AUDIO:
    val = is->audio_clock->GetClock();
    break;
  default:
    val = is->external_clock->GetClock();
    break;
  }
  return val;
}

static inline int getMasterClockSerial(const sp<VideoState> is) {
  int val = -1;

  if (!is) {
    return val;
  }

  switch (getMasterSyncType(is)) {
  case CLOCK_VIDEO:
    val = is->video_clock->GetClockSerial();
    break;
  case CLOCK_AUDIO:
    val = is->audio_clock->GetClockSerial();
    break;
  default:
    val = is->external_clock->GetClockSerial();
    break;
  }
  return val;
}

static inline bool getMasterClockAvaliable(const sp<VideoState> is) {
  bool val = false;

  if (!is) {
    return val;
  }

  switch (getMasterSyncType(is)) {
  case CLOCK_VIDEO:
    val = is->video_clock->GetClockAvaliable();
    break;
  case CLOCK_AUDIO:
    val = is->audio_clock->GetClockAvaliable();
    break;
  default:
    val = is->external_clock->GetClockAvaliable();
    break;
  }
  return val;
}

static inline void setMasterClockAvaliable(const sp<VideoState> is,
                                           bool avaliable) {

  if (!is) {
    return;
  }

  switch (getMasterSyncType(is)) {
  case CLOCK_VIDEO:
    is->video_clock->SetClockAvaliable(avaliable);
    break;
  case CLOCK_AUDIO:
    is->audio_clock->SetClockAvaliable(avaliable);
    break;
  default:
    is->external_clock->SetClockAvaliable(avaliable);
    break;
  }
}

REDPLAYER_NS_END;
