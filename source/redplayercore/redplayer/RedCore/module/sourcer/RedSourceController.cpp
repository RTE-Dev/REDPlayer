/*
 * RedSourceController.cpp
 *
 *  Created on: 2022年8月19日
 *      Author: liuhongda
 */

#include "RedCore/module/sourcer/RedSourceController.h"
#include "RedError.h"
#include "RedMsg.h"
#include "RedProp.h"
#include "base/RedConfig.h"
#include "wrapper/reddownload_datasource_wrapper.h"

#define TAG "RedSourceController"

REDPLAYER_NS_BEGIN;

constexpr int kMinBufferingNotifyStep = 10;
constexpr int kMaxRetryCount = 3;

CRedSourceController::CRedSourceController(int id, const sp<VideoState> &state,
                                           NotifyCallback notify_cb)
    : mID(id), mVideoState(state), mNotifyCb(notify_cb) {}

CRedSourceController::~CRedSourceController() { mRedSource.reset(); }

void CRedSourceController::setNotifyCb(NotifyCallback notify_cb) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  mNotifyCb = notify_cb;
}

void CRedSourceController::setPrepareCb(PrepareCallBack cb) {
  std::unique_lock<std::mutex> lck(mPreparedCbLock);
  mPrepareCb = cb;
}

void CRedSourceController::release() {
  AV_LOGD_ID(TAG, mID, "%s start\n", __func__);
  mAbort = true;
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      AV_LOGD_ID(TAG, mID, "%s already released, just return.\n", __func__);
      return;
    }
    mReleased = true;
  }
  if (mThread.joinable()) {
    mThread.join();
  }
  for (auto iter = mPktQueueMap.begin(); iter != mPktQueueMap.end(); ++iter) {
    iter->second->clear();
  }
}

RED_ERR CRedSourceController::putPacket(std::unique_ptr<RedAvPacket> &pkt,
                                        int stream_type) {
  sp<PktQueue> pktqueue = pktQueue(stream_type);
  if (!pktqueue) {
    return OK;
  }
  return pktqueue->putPkt(pkt);
}

RED_ERR CRedSourceController::getPacket(std::unique_ptr<RedAvPacket> &pkt,
                                        int stream_type, bool block) {
  sp<PktQueue> pktqueue = pktQueue(stream_type);
  if (!pktqueue) {
    return ME_ERROR;
  }
  RED_ERR ret = pktqueue->getPkt(pkt, block);
  updateCacheStatistic();
  return ret;
}

bool CRedSourceController::pktQueueFrontIsFlush(int stream_type) {
  sp<PktQueue> pktqueue = pktQueue(stream_type);
  if (!pktqueue) {
    return false;
  }
  return pktqueue->frontIsFlush();
}

void CRedSourceController::pktQueueAbort(int stream_type) {
  sp<PktQueue> pktqueue = pktQueue(stream_type);
  if (!pktqueue) {
    return;
  }
  return pktqueue->abort();
}

RED_ERR CRedSourceController::open(std::string url) {
  std::unique_lock<std::mutex> lck(mLock);
  mUrl = url;
  return OK;
}

RED_ERR CRedSourceController::seek(int64_t msec) {
  std::unique_lock<std::mutex> lck(mLock);
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  if (!mVideoState->seek_req) {
    mVideoState->seek_pos = msec * 1000;
    mVideoState->seek_req = true;
    mCond.notify_one();
  }
  return OK;
}

RED_ERR CRedSourceController::prepareAsync() {
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      return OK;
    }
    this->run();
  }
  return OK;
}

RED_ERR CRedSourceController::PerformStop() {
  std::unique_lock<std::mutex> lck(mLock);
  RED_ERR ret = OK;
  mAbort = true;
  if (mRedSource) {
    mRedSource->setInterrupt();
  }
  for (auto iter = mPktQueueMap.begin(); iter != mPktQueueMap.end(); ++iter) {
    iter->second->abort();
  }
  updateCacheStatistic();
  mCond.notify_all();
  return ret;
}

RED_ERR CRedSourceController::PerformFlush() {
  ++mSerial;
  if (mPktQueueMap.empty())
    return OK;
  for (auto iter = mPktQueueMap.begin(); iter != mPktQueueMap.end(); ++iter) {
    iter->second->flush();
  }
  updateCacheStatistic();
  AV_LOGD_ID(TAG, mID, "%s serial %d\n", __func__, mSerial);
  return OK;
}

// @return true if buffer is not full, false if buffer is full.
bool CRedSourceController::isBufferFull() {
  return ((!mVideoState->seek_req &&
           mVideoState->stat.audio_cache.bytes +
                   mVideoState->stat.video_cache.bytes >
               mMaxBufferSize &&
           (mVideoState->stat.audio_cache.bytes > 64000 ||
            mMetaData->audio_index < 0) &&
           (mVideoState->stat.video_cache.bytes > 256000 ||
            mMetaData->video_index < 0)) ||
          ((mVideoState->stat.audio_cache.packets >= DEFAULT_MIN_FRAMES ||
            mMetaData->audio_index < 0) &&
           (mVideoState->stat.video_cache.packets >= DEFAULT_MIN_FRAMES ||
            mMetaData->video_index < 0)));
}

void CRedSourceController::toggleBuffering(bool buffering) {
  std::unique_lock<std::mutex> lck(mLock);
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (!player_config->packet_buffering) {
    return;
  }

  if (buffering && !mBuffering && !mEOF && !mVideoState->pause_req) {
    AV_LOGI_ID(TAG, mID, "%s: start\n", __func__);
    mBuffering = true;
    mBufferingPercent = 0;
    if (mVideoState->seek_req ||
        mVideoState->latest_audio_seek_load_serial >= 0 ||
        mVideoState->latest_video_seek_load_serial >= 0) {
      mSeekBuffering = true;
      notifyListener(RED_MSG_BUFFERING_START, 1);
    } else {
      notifyListener(RED_MSG_BUFFERING_START, 0);
    }
  } else if (!buffering && mBuffering) {
    AV_LOGI_ID(TAG, mID, "%s: end %d %d\n", __func__,
               mVideoState->first_video_frame_rendered,
               mVideoState->first_audio_frame_rendered);
    mBuffering = false;
    if (mSeekBuffering || mVideoState->latest_audio_seek_load_serial >= 0 ||
        mVideoState->latest_video_seek_load_serial >= 0) {
      mSeekBuffering = false;
      notifyListener(RED_MSG_BUFFERING_END, 1);
    } else {
      notifyListener(RED_MSG_BUFFERING_END, 0);
    }
  }
}

bool CRedSourceController::isBuffering() {
  std::unique_lock<std::mutex> lck(mLock);
  return mBuffering && !mEOF;
}

int CRedSourceController::getSerial() {
  std::unique_lock<std::mutex> lck(mLock);
  return mSerial;
}

void CRedSourceController::checkBuffering() {
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  int video_index = -1, audio_index = -1;
  if (!player_config->packet_buffering) {
    return;
  }
  if (!isBuffering()) {
    return;
  }
  if (mMetaData) {
    video_index = mMetaData->video_index;
    audio_index = mMetaData->audio_index;
  }

  int hwm_in_ms =
      player_config->dcc.current_high_water_mark_in_ms; // use fast water mark
                                                        // for first loading
  int buf_size_percent = -1;
  int buf_time_percent = -1;
  int hwm_in_bytes = player_config->dcc.high_water_mark_in_bytes;
  bool need_start_buffering = false;
  bool audio_time_base_valid = false;
  bool video_time_base_valid = false;
  int64_t buf_time_position = -1;
  TrackInfo audio_track_info, video_track_info;

  if (audio_index >= 0) {
    audio_track_info = mMetaData->track_info[audio_index];
    audio_time_base_valid = audio_track_info.time_base_num > 0 &&
                            audio_track_info.time_base_den > 0;
  }
  if (video_index >= 0) {
    video_track_info = mMetaData->track_info[video_index];
    video_time_base_valid = video_track_info.time_base_num > 0 &&
                            video_track_info.time_base_den > 0;
  }

  if (hwm_in_ms > 0) {
    int cached_duration_in_ms = -1;
    int64_t audio_cached_duration = -1;
    int64_t video_cached_duration = -1;

    if (audio_time_base_valid) {
      audio_cached_duration = mVideoState->stat.audio_cache.duration;
    }

    if (video_time_base_valid) {
      video_cached_duration = mVideoState->stat.video_cache.duration;
    }

    if (video_cached_duration > 0 && audio_cached_duration > 0) {
      cached_duration_in_ms =
          std::min(video_cached_duration, audio_cached_duration);
    } else if (video_cached_duration > 0) {
      cached_duration_in_ms = static_cast<int>(video_cached_duration);
    } else if (audio_cached_duration > 0) {
      cached_duration_in_ms = static_cast<int>(audio_cached_duration);
    }

    if (cached_duration_in_ms >= 0) {
      mVideoState->playable_duration_ms =
          mVideoState->current_position_ms + cached_duration_in_ms;
      buf_time_percent = static_cast<int>(
          av_rescale(cached_duration_in_ms, 1005, hwm_in_ms * 10));
    }
  }

  int cached_size =
      mVideoState->stat.audio_cache.bytes + mVideoState->stat.video_cache.bytes;
  if (hwm_in_bytes > 0) {
    buf_size_percent =
        static_cast<int>(av_rescale(cached_size, 1005, hwm_in_bytes * 10));
  }

  int buf_percent = -1;
  if (buf_time_percent >= 0) {
    // always depend on cache duration if valid
    if (buf_time_percent >= 100)
      need_start_buffering = true;
    buf_percent = buf_time_percent;
  } else {
    if (buf_size_percent >= 100)
      need_start_buffering = true;
    buf_percent = buf_size_percent;
  }

  if (buf_time_percent >= 0 && buf_size_percent >= 0) {
    buf_percent = std::min(buf_time_percent, buf_size_percent);
  }
  if (buf_percent) {
    if (buf_percent - mBufferingPercent >= kMinBufferingNotifyStep) {
      notifyListener(RED_MSG_BUFFERING_UPDATE,
                     static_cast<int>(buf_time_position), buf_percent);
      mBufferingPercent = buf_percent;
    }
  }

  if (need_start_buffering) {
    if (hwm_in_ms < player_config->dcc.next_high_water_mark_in_ms) {
      hwm_in_ms = player_config->dcc.next_high_water_mark_in_ms;
    } else {
      hwm_in_ms *= 2;
    }

    if (hwm_in_ms > player_config->dcc.last_high_water_mark_in_ms)
      hwm_in_ms = player_config->dcc.last_high_water_mark_in_ms;

    player_config->dcc.current_high_water_mark_in_ms = hwm_in_ms;

    if ((mVideoState->stat.audio_cache.packets >= MIN_MIN_FRAMES ||
         mMetaData->audio_index < 0) &&
        (mVideoState->stat.video_cache.packets >= MIN_MIN_FRAMES ||
         mMetaData->video_index < 0)) {
      toggleBuffering(false);
    }
  }
}

bool CRedSourceController::checkDropNonRefFrame(AVPacket *pkt) {
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  int video_index = -1;
  if (mMetaData) {
    video_index = mMetaData->video_index;
  }
  if (video_index < 0) {
    return false;
  }
  TrackInfo track_info = mMetaData->track_info[video_index];
  AVRational tb =
      (AVRational){track_info.time_base_num, track_info.time_base_den};
  if (player_config->enable_accurate_seek &&
      mVideoState->video_accurate_seek_req && !mVideoState->seek_req &&
      !mVideoState->is_video_high_fps &&
      mVideoState->skip_frame >= DISCARD_NONREF) {
    double f_pts = (pkt->pts == AV_NOPTS_VALUE) ? 0.0 : pkt->pts * av_q2d(tb);
    double f_dts = (pkt->dts == AV_NOPTS_VALUE) ? 0.0 : pkt->dts * av_q2d(tb);
    double f_seek_pos = mVideoState->seek_pos / 1000000.0;
    if (f_pts > f_seek_pos || f_dts > f_seek_pos) {
      mVideoState->skip_frame =
          std::min(mVideoState->skip_frame, static_cast<int>(DISCARD_DEFAULT));
    }
  }
  mVideoState->stat.total_packet_count++;
  if (mVideoState->nal_length_size &&
      pkt->size > mVideoState->nal_length_size + 1 &&
      mVideoState->skip_frame >= DISCARD_NONREF) {
    int nal_unit_type = 0;
    int nuh_layer_id = 0;
    int ref_idc = 0;
    if (track_info.codec_id == AV_CODEC_ID_H265) {
      nal_unit_type = (pkt->data[mVideoState->nal_length_size] >> 1) & 0x3F;
      nuh_layer_id = ((pkt->data[mVideoState->nal_length_size] & 0x01) << 5) +
                     (pkt->data[mVideoState->nal_length_size + 1] >> 3);
      if (IsHevcNalNonRef(nal_unit_type) || nuh_layer_id > 0) {
        mVideoState->stat.drop_packet_count++;
        return true;
      }
    } else if (track_info.codec_id == AV_CODEC_ID_H264) {
      ref_idc = (pkt->data[mVideoState->nal_length_size] >> 5) & 0x03;
      nal_unit_type = pkt->data[mVideoState->nal_length_size] & 0x1F;
      if (ref_idc == 0 && nal_unit_type != NAL_SEI) {
        if (pkt->flags & AV_PKT_FLAG_KEY) {
          mVideoState->skip_frame = std::min(mVideoState->skip_frame,
                                             static_cast<int>(DISCARD_DEFAULT));
          return false;
        }
        mVideoState->stat.drop_packet_count++;
        return true;
      }
    }
  }
  return false;
}

void CRedSourceController::updateCacheStatistic() {
  sp<PktQueue> vq = pktQueue(TYPE_VIDEO);
  if (vq) {
    if (mMetaData) {
      int video_index = mMetaData->video_index;
      if (video_index >= 0) {
        auto track_info = mMetaData->track_info[video_index];
        AVRational tb =
            (AVRational){track_info.time_base_num, track_info.time_base_den};
        mVideoState->stat.video_cache.duration =
            (vq->duration() * av_q2d(tb) * 1000);
      }
    }
    mVideoState->stat.video_cache.bytes = vq->bytes();
    mVideoState->stat.video_cache.packets = vq->size();
  }
  sp<PktQueue> aq = pktQueue(TYPE_AUDIO);
  if (aq) {
    if (mMetaData) {
      int audio_index = mMetaData->audio_index;
      if (audio_index >= 0) {
        auto track_info = mMetaData->track_info[audio_index];
        AVRational tb =
            (AVRational){track_info.time_base_num, track_info.time_base_den};
        mVideoState->stat.audio_cache.duration =
            (aq->duration() * av_q2d(tb) * 1000);
      }
    }
    mVideoState->stat.audio_cache.bytes = aq->bytes();
    mVideoState->stat.audio_cache.packets = aq->size();
  }
}

void CRedSourceController::notifyListener(uint32_t what, int32_t arg1,
                                          int32_t arg2, void *obj1, void *obj2,
                                          int obj1_len, int obj2_len) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  if (mNotifyCb) {
    mNotifyCb(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

sp<PktQueue> CRedSourceController::pktQueue(int stream_type) {
  sp<PktQueue> q;
  auto iter = mPktQueueMap.begin();
  if ((iter = mPktQueueMap.find(stream_type)) != mPktQueueMap.end()) {
    q = mPktQueueMap[stream_type];
  }
  return q;
}

void CRedSourceController::setConfig(const sp<CoreGeneralConfig> &config) {
  std::unique_lock<std::mutex> lck(mLock);
  mGeneralConfig = config;
}

RED_ERR CRedSourceController::SetMetaData() {
  std::unique_lock<std::mutex> lck(mLock);
  for (auto iter = mMetaData->track_info.begin();
       iter != mMetaData->track_info.end(); ++iter) {
    if (iter->stream_type == TYPE_AUDIO && mAudioIndex == -1) {
      mAudioIndex = iter->stream_index;
      mPktQueueMap[TYPE_AUDIO] = std::make_shared<PktQueue>(TYPE_AUDIO);
    } else if (iter->stream_type == TYPE_VIDEO && mVideoIndex == -1) {
      mVideoIndex = iter->stream_index;
      mPktQueueMap[TYPE_VIDEO] = std::make_shared<PktQueue>(TYPE_VIDEO);
    }
  }
  mMetaData->audio_index = mAudioIndex;
  mMetaData->video_index = mVideoIndex;
  lck.unlock();
  std::unique_lock<std::mutex> lck1(mPreparedCbLock);
  if (mPrepareCb) {
    mPrepareCb(mMetaData);
  }
  return OK;
}

RED_ERR CRedSourceController::putFlushPacket() {
  if (mPktQueueMap.size() == 0) {
    AV_LOGE_ID(TAG, mID, "[%s][%d] none pktqueue! \n", __FUNCTION__, __LINE__);
    return ME_ERROR;
  }
  for (auto iter = mPktQueueMap.begin(); iter != mPktQueueMap.end(); ++iter) {
    std::unique_ptr<RedAvPacket> avpkt(new RedAvPacket(PKT_TYPE_FLUSH));
    iter->second->putPkt(avpkt);
  }
  return OK;
}

RED_ERR CRedSourceController::putEofPacket() {
  if (mPktQueueMap.size() == 0) {
    AV_LOGE_ID(TAG, mID, "[%s][%d] none pktqueue! \n", __FUNCTION__, __LINE__);
    return ME_ERROR;
  }
  for (auto iter = mPktQueueMap.begin(); iter != mPktQueueMap.end(); ++iter) {
    std::unique_ptr<RedAvPacket> avpkt(new RedAvPacket(PKT_TYPE_EOF));
    iter->second->putPkt(avpkt);
  }
  return OK;
}

// base method
void CRedSourceController::ThreadFunc() {
  AVPacket *pkt = av_packet_alloc();
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  redsource::FFMpegOpt opt{0};
  int64_t prev_buffer_check_time = 0;
  int64_t buffer_check_time = 0;
  int connect_retry_count = kMaxRetryCount;
  int read_retry_count = kMaxRetryCount;
  int ret = 0;
  int error_type = ERROR_UNKOWN;
  bool completed = false;
#if defined(__APPLE__)
  pthread_setname_np("readthread");
#elif defined(__ANDROID__) || defined(__HARMONY__)
  pthread_setname_np(pthread_self(), "readthread");
#endif
  AV_LOGI_ID(TAG, mID, "RedSource thread Start.");

  mMaxBufferSize = player_config->dcc.max_buffer_size > 0
                       ? player_config->dcc.max_buffer_size
                       : mMaxBufferSize;

  notifyListener(RED_MSG_BPREPARED);

  do {
    {
      std::unique_lock<std::mutex> lck(mNotifyCbLock);
      mRedSource = std::make_shared<CRedSource>(mID, mNotifyCb);
    }
    mMetaData = std::make_shared<MetaData>();
    if (!mRedSource || !mMetaData) {
      AV_LOGE_ID(TAG, mID, "[%s][%d]create redsource failed \n", __FUNCTION__,
                 __LINE__);
      notifyListener(RED_MSG_ERROR, ERROR_REDPLAYER_CREATE);
      return;
    }
    if (connect_retry_count <= 0) {
      const std::string reddownloadPrefix = "httpreddownload:";
      if (reddownloadPrefix.size() <= mUrl.size() &&
          strncmp(reddownloadPrefix.c_str(), mUrl.c_str(),
                  reddownloadPrefix.length()) == 0) {
        mUrl = mUrl.substr(reddownloadPrefix.length());
      }
    }
    av_dict_copy(&opt.format_opts, mGeneralConfig->formatConfig, 0);
    av_dict_copy(&opt.codec_opts, mGeneralConfig->codecConfig, 0);
    AV_LOGD_ID(TAG, mID, "open %s for read\n", mUrl.c_str());
    ret = mRedSource->open(mUrl, opt, mMetaData);
    av_dict_free(&opt.format_opts);
    av_dict_free(&opt.codec_opts);
  } while (ret < 0 && !mAbort && connect_retry_count-- > 0);

  if (ret != OK) {
    AV_LOGE_ID(TAG, mID, "[%s][%d]open redsource failed \n", __FUNCTION__,
               __LINE__);
    mRedSource->close();
    if ((error_type = getErrorType(ret)) == ERROR_UNKOWN) {
      notifyListener(RED_MSG_ERROR, ERROR_OPEN_INPUT, ret);
    } else {
      notifyListener(RED_MSG_ERROR, error_type, ret);
    }
    return;
  } else {
    SetMetaData();
  }

  while (!mAbort) {
    if (mVideoState->seek_req) {
      mEOF = false;
      toggleBuffering(true);
      notifyListener(RED_MSG_BUFFERING_UPDATE, 0, 0);
      int ret = mRedSource->seek(mVideoState->seek_pos);
      if (ret < 0) {
        AV_LOGE_ID(TAG, mID, "%s Error while seeking\n", mUrl.c_str());
      } else {
        AV_LOGD_ID(TAG, mID, "%s seek to %" PRId64 " success\n", mUrl.c_str(),
                   mVideoState->seek_pos);
        {
          PerformFlush();
          putFlushPacket();
        }
        mVideoState->latest_video_seek_load_serial = mSerial;
        mVideoState->latest_audio_seek_load_serial = mSerial;
        mVideoState->latest_seek_load_start_at = CurrentTimeUs();
        setMasterClockAvaliable(mVideoState, false);
      }
      mVideoState->seek_req = false;
      player_config->dcc.current_high_water_mark_in_ms =
          player_config->dcc.first_high_water_mark_in_ms;
      if (player_config->enable_accurate_seek) {
        mVideoState->drop_aframe_count = 0;
        mVideoState->drop_vframe_count = 0;
        std::unique_lock<std::mutex> lck(mVideoState->accurate_seek_mutex);
        if (mMetaData->video_index >= 0) {
          if (mVideoState->skip_frame < DISCARD_NONREF) {
            mVideoState->skip_frame = std::max(
                mVideoState->skip_frame, static_cast<int>(DISCARD_NONREF));
          }
          mVideoState->video_accurate_seek_req = true;
        }
        if (mMetaData->audio_index >= 0) {
          mVideoState->audio_accurate_seek_req = true;
        }
        mVideoState->audio_accurate_seek_cond.notify_one();
        mVideoState->video_accurate_seek_cond.notify_one();
      }
      notifyListener(RED_MSG_SEEK_COMPLETE, mVideoState->seek_pos / 1000);
      completed = false;
      toggleBuffering(true);
      continue;
    }

    updateCacheStatistic();

    if (isBufferFull()) {
      toggleBuffering(false);
      usleep(10 * 1000);
      continue;
    }
    if ((!mVideoState->paused || completed) &&
        (mMetaData->audio_index < 0 || mVideoState->auddec_finished) &&
        (mMetaData->video_index < 0 || mVideoState->viddec_finished)) {
      if (completed) {
        std::unique_lock<std::mutex> lck(mLock);
        while (!mAbort && !mVideoState->seek_req) {
          mCond.wait_for(lck, std::chrono::milliseconds(100));
        }
        if (!mAbort) {
          continue;
        }
      } else {
        completed = true;
        toggleBuffering(false);
        AV_LOGI_ID(TAG, mID, "Completed, error %d\n", mVideoState->error);
        if (mVideoState->error != 0) {
          notifyListener(RED_REQ_INTERNAL_PAUSE);
          notifyListener(RED_MSG_ERROR, ERROR_READ_FRAME, mVideoState->error);
        } else {
          notifyListener(RED_MSG_COMPLETED);
        }
      }
    }
    RED_ERR ret = mRedSource->readPacket(pkt);
    if (ret != OK || !pkt) {
      if (ret == AVERROR_EOF || ret == AVERROR_EXIT ||
          mRedSource->getPbError()) {
        if (!mEOF) {
          AV_LOGI_ID(TAG, mID, "EOF!\n");
          mEOF = true;
          putEofPacket();
        }

        toggleBuffering(false);

        if (mRedSource->getPbError()) {
          mVideoState->error = mRedSource->getPbError();
        }
        if (ret == AVERROR_EXIT) {
          mVideoState->error = AVERROR_EXIT;
        }

        std::unique_lock<std::mutex> lck(mLock);
        if (!mAbort && !mVideoState->seek_req) {
          mCond.wait_for(lck, std::chrono::milliseconds(100));
        }
        continue;
      } else {
        if (mAbort) {
          continue;
        }
        if (read_retry_count-- > 0) {
          usleep(10 * 1000);
          continue;
        }
        AV_LOGE_ID(TAG, mID, "read thread retry failed\n");
        if ((error_type = getErrorType(ret)) == ERROR_UNKOWN) {
          notifyListener(RED_MSG_ERROR, ERROR_READ_FRAME, ret);
        } else {
          notifyListener(RED_MSG_ERROR, error_type, ret);
        }
        std::unique_lock<std::mutex> lck(mLock);
        if (!mAbort) {
          mCond.wait(lck);
        }
        continue;
      }
    } else {
      mVideoState->error = 0;
      mEOF = false;
    }
    if (pkt->stream_index == mAudioIndex) {
      std::unique_ptr<RedAvPacket> avpkt(new RedAvPacket(pkt, mSerial));
      putPacket(avpkt, TYPE_AUDIO);
    } else if (pkt->stream_index == mVideoIndex && !checkDropNonRefFrame(pkt)) {
      std::unique_ptr<RedAvPacket> avpkt(new RedAvPacket(pkt, mSerial));
      putPacket(avpkt, TYPE_VIDEO);
      if (!mFirstVideoPktInPktQueue) {
        mFirstVideoPktInPktQueue = true;
      }
    }
    updateCacheStatistic();
    buffer_check_time = CurrentTimeMs();
    if ((mVideoState->first_video_frame_rendered != 1 &&
         mMetaData->video_index >= 0) ||
        (!mVideoState->first_audio_frame_rendered &&
         mMetaData->audio_index >= 0)) {
      prev_buffer_check_time = buffer_check_time;
      player_config->dcc.current_high_water_mark_in_ms =
          player_config->dcc.first_high_water_mark_in_ms;
      checkBuffering();
    } else {
      if (std::abs(buffer_check_time - prev_buffer_check_time) >
          BUFFERING_CHECK_PER_MILLISECONDS) {
        prev_buffer_check_time = buffer_check_time;
        checkBuffering();
      }
    }
    av_packet_unref(pkt);
  }
  if (pkt) {
    av_packet_free(&pkt);
  }
  if (mRedSource) {
    mRedSource->close();
  }
  AV_LOGD_ID(TAG, mID, "RedSource readthread Exit.");
}

RED_ERR CRedSourceController::stop() { return PerformStop(); }

int CRedSourceController::getErrorType(int errorCode) {
  switch (errorCode) {
  case AVERROR_HTTP_BAD_REQUEST:
  case AVERROR_HTTP_UNAUTHORIZED:
  case AVERROR_HTTP_FORBIDDEN:
  case AVERROR_HTTP_NOT_FOUND:
  case AVERROR_HTTP_OTHER_4XX:
  case AVERROR_HTTP_SERVER_ERROR:
    return ERROR_HTTP;
#if defined(__ANDROID__)
  case AVERROR_INVALIDDATA:
    return ERROR_INVALID_DATA;
#endif
  }
  return ERROR_UNKOWN;
}

REDPLAYER_NS_END;
