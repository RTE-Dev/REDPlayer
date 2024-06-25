/*
 * RedCore.cpp
 *
 *  Created on: 2022年7月25日
 *      Author: liuhongda
 */

#include "RedCore/RedCore.h"
#include "RedCore/module/sourcer/format/redioapplication.h"
#include "RedMsg.h"
#include "wrapper/reddownload_datasource_wrapper.h"
#include <sys/socket.h>

#define TAG "RedCore"

REDPLAYER_NS_BEGIN;

static InjectCallback s_inject_callback;
int inject_callback(void *opaque, int type, void *data) {
  if (s_inject_callback)
    return s_inject_callback(opaque, type, data);
  return 0;
}

void globalSetInjectCallback(InjectCallback cb) { s_inject_callback = cb; }

static int app_func_event(RedApplicationContext *h, int message, void *data) {
  if (!h || !h->opaque || !data)
    return 0;

  CRedCore *core = reinterpret_cast<CRedCore *>(h->opaque);
  sp<VideoState> state = core->mVideoState;
  if (!core->getInjectOpaque() || !state)
    return 0;
  if (message == RED_EVENT_IO_TRAFFIC_W) {
    RedIOTrafficWrapper *event = reinterpret_cast<RedIOTrafficWrapper *>(data);
    if (event->bytes > 0) {
      state->stat.byte_count += event->bytes;
      if (state->stat.isaf_inet6) {
        state->stat.byte_count_inet6 += event->bytes;
      } else {
        state->stat.byte_count_inet += event->bytes;
      }
      state->stat.tcp_read_sampler.add(event->bytes);
    }
  } else if (message == RED_CTRL_DID_TCP_OPEN_W) {
    state->stat.tcp_read_sampler.reset(RED_TCP_READ_SAMPLE_RANGE);
  } else if (message == RED_EVENT_CACHE_STATISTIC_W) {
    RedCacheStatisticWrapper *statistic =
        reinterpret_cast<RedCacheStatisticWrapper *>(data);
    if (state->stat.real_cached_size ==
        -1) { // only update the real_cached_size once in every play
      if (statistic->cached_size < 0) {
        state->stat.real_cached_size = 0;
      } else {
        state->stat.real_cached_size = statistic->cached_size;
      }
    }

    if (statistic->cached_size <= 0 || statistic->logical_file_size <= 0) {
      return 0;
    }
    state->stat.cache_physical_pos = statistic->cache_physical_pos;
    state->stat.cache_file_forwards = statistic->cache_file_forwards;
    state->stat.cache_file_pos = statistic->cache_file_pos;
    state->stat.cache_count_bytes = statistic->cache_count_bytes;
    state->stat.logical_file_size = statistic->logical_file_size;
    state->stat.cached_size = statistic->cached_size;

    int64_t cached_size = statistic->cached_size;
    int64_t file_size = statistic->logical_file_size;
    if ((cached_size < file_size &&
         cached_size - state->stat.last_cached_pos < file_size * 0.02) ||
        state->stat.last_cached_pos == file_size) {
      return 0;
    }
    state->stat.last_cached_pos = cached_size;
  }
  switch (message) {
  case RED_CTRL_DID_TCP_OPEN_W: {
    RedTcpIOControlWrapper *real_data =
        reinterpret_cast<RedTcpIOControlWrapper *>(data);
    if (real_data) {
      state->stat.isaf_inet6 = (real_data->family == AF_INET6);
      AV_LOGD_ID(TAG, core->id(), "tcp open is ipv6 %d , %d--%d, ip %s\n",
                 state->stat.isaf_inet6, real_data->family, AF_INET6,
                 real_data->ip);
    }
    break;
  }
  case RED_EVENT_URL_CHANGE_W: {
    RedUrlChangeEventWrapper *event =
        reinterpret_cast<RedUrlChangeEventWrapper *>(data);
    AV_LOGI_ID(TAG, core->id(),
               "RED_EVENT_URL_CHANGE_W http_error_code = %d, current_url = %s, "
               "next_url = %s\n",
               event->http_code, event->current_url, event->next_url);
    if (!state->backup_url_is_null) {
      core->notifyListener(RED_MSG_URL_CHANGE, event->http_code, 0,
                           event->current_url, event->next_url,
                           MAX_URL_LIST_SIZE, MAX_URL_LIST_SIZE);
    }
    if (strlen(event->next_url) == 0)
      state->backup_url_is_null = true;
    break;
  }
  default:
    break;
  }
  return inject_callback(core->getInjectOpaque(), message, data);
}

CRedCore::CRedCore(int id, NotifyCallback notify_cb)
    : mID(id), mNotifyCb(notify_cb) {}

CRedCore::~CRedCore() {
  mRedRenderVideoHal.reset();
  mRedRenderAudioHal.reset();
  mVideoProcesser.reset();
  mAudioProcesser.reset();
  mRedSourceController.reset();
  resetConfigs();
}

int CRedCore::id() const { return mID; }

RED_ERR CRedCore::init() {
  try {
    mVideoState = std::make_shared<VideoState>();
    mVideoState->audio_clock = std::make_unique<RedClock>();
    mVideoState->video_clock = std::make_unique<RedClock>();
    mVideoState->external_clock = std::make_unique<RedClock>();
    mGeneralConfig = std::make_shared<CoreGeneralConfig>();
    mGeneralConfig->playerConfig = std::make_shared<RedPlayerConfig>();
    mFormatConfig = std::make_shared<RedDict>();
    mCodecConfig = std::make_shared<RedDict>();
    mSwsConfig = std::make_shared<RedDict>();
    mSwrConfig = std::make_shared<RedDict>();
    mPlayerConfig = std::make_shared<RedDict>();
    auto notify_cb = std::bind(
        &CRedCore::notifyListener, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
        std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
    mRedSourceController =
        std::make_shared<CRedSourceController>(mID, mVideoState, notify_cb);
    mVideoProcesser = std::make_shared<CVideoProcesser>(
        mID, mRedSourceController, mVideoState, notify_cb);
    mAudioProcesser = std::make_shared<CAudioProcesser>(
        mID, mRedSourceController, mVideoState, notify_cb);
    mRedRenderAudioHal = std::make_shared<CRedRenderAudioHal>(
        mID, mAudioProcesser, mVideoState, notify_cb);
    mRedRenderVideoHal = std::make_shared<CRedRenderVideoHal>(
        mID, mVideoProcesser, mVideoState, notify_cb);
    mAppCtx = reinterpret_cast<RedApplicationContext *>(
        malloc(sizeof(RedApplicationContext)));
  } catch (const std::bad_alloc &e) {
    AV_LOGE(TAG, "[%s:%d] Exception caught: %s!\n", __FUNCTION__, __LINE__,
            e.what());
    return NO_MEMORY;
  } catch (...) {
    AV_LOGE(TAG, "[%s:%d] Exception caught!\n", __FUNCTION__, __LINE__);
    return NO_MEMORY;
  }

  return OK;
}

void CRedCore::setDataSource(std::string url) {
  std::unique_lock<std::mutex> lck(mLock);
  mUrl = url;
}

void CRedCore::setDataSourceFd(int64_t fd) {
  std::unique_lock<std::mutex> lck(mLock);
  if (mFd >= 0) {
    close(mFd);
    mFd = -1;
  }
  mFd = dup(fd);
  char url[128];
  snprintf(url, sizeof(url), "pipe:%" PRId64 "", mFd);
  mUrl = url;
}

RED_ERR CRedCore::prepareAsync() {
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  auto source_controller = mRedSourceController;
  if (mUrl.empty() || !source_controller) {
    return ME_ERROR;
  }

  PerformConfigs();
  bool enable_reddownload =
      ((mUrl.substr(0, 5) == "http:") || (mUrl.substr(0, 6) == "https:")) &&
      (mUrl.find(".m3u") == std::string::npos);
  if (enable_reddownload) {
    std::string newUrl;
    const std::string reddownloadPrefix = "httpreddownload:";
    const std::string httpPrefix = "http";
    if (mUrl.find(httpPrefix) == 0 &&
        mUrl.find(reddownloadPrefix) == std::string::npos) {
      newUrl = reddownloadPrefix + mUrl;
    } else {
      newUrl = mUrl;
    }
    mUrl = newUrl;
  }
  mPausedByClient = !player_config->start_on_prepared;
  source_controller->open(mUrl);
  return PerformPrepare();
}
RED_ERR CRedCore::start() {
  std::unique_lock<std::mutex> lck(mLock);
  mPausedByClient = false;
  return PerformStart();
}
RED_ERR CRedCore::startFrom(int64_t msec) {
  seekTo(msec);
  return start();
}
RED_ERR CRedCore::pause() {
  std::unique_lock<std::mutex> lck(mLock);
  mPausedByClient = true;
  return PerformPause();
}
RED_ERR CRedCore::seekTo(int64_t msec, bool flush_queue) {
  std::unique_lock<std::mutex> lck(mLock);
  int64_t duration = mMetaData ? mMetaData->duration / 1000 : 0;

  if (duration > 0 && msec >= duration) {
    notifyListener(RED_MSG_COMPLETED);
    return OK;
  }

  mCompleted.store(false);
  mCurSeekPos.store(msec);
  return PerformSeek(msec);
}
RED_ERR CRedCore::stop() {
  std::unique_lock<std::mutex> lck(mLock);
  mAbort = true;
  return PerformStop();
}

RED_ERR CRedCore::getCurrentPosition(int64_t &pos_ms) {
  auto source_controller = mRedSourceController;
  int64_t start_time_ms = 0;
  if (mMetaData && mMetaData->start_time > 0) {
    start_time_ms = mMetaData->start_time / 1000;
  }
  if (mCompleted.load()) {
    pos_ms = mMetaData->duration / 1000;
    return OK;
  } else if (source_controller &&
             getMasterClockSerial(mVideoState) ==
                 source_controller->getSerial() &&
             !mSeeking.load()) {
    double pos = getMasterClock(mVideoState);
    pos_ms = pos > 0.0 ? pos * 1000 : 0;
  } else {
    int64_t curSeekPos = mCurSeekPos.load();
    pos_ms = curSeekPos > 0 ? curSeekPos : 0;
    return OK;
  }
  if (pos_ms < 0 || pos_ms < start_time_ms) {
    pos_ms = 0;
    return OK;
  }
  if (pos_ms > start_time_ms) {
    pos_ms -= start_time_ms;
  }
  mVideoState->current_position_ms = pos_ms;
  return OK;
}

RED_ERR CRedCore::getDuration(int64_t &dur_ms) {
  if (!mMetaData) {
    dur_ms = 0;
    return NO_INIT;
  }
  dur_ms = mMetaData->duration / 1000;
  return OK;
}

void CRedCore::setVolume(const float left_volume, const float right_volume) {
  std::unique_lock<std::mutex> lck(mAudioStreamLock);
  auto render_audio_hal = mRedRenderAudioHal;
  lck.unlock();
  if (render_audio_hal) {
    render_audio_hal->setVolume(left_volume, right_volume);
  }
  mVideoState->volume = (left_volume + right_volume) / 2;
}

void CRedCore::setMute(bool mute) {
  std::unique_lock<std::mutex> lck(mAudioStreamLock);
  auto render_audio_hal = mRedRenderAudioHal;
  lck.unlock();
  if (render_audio_hal) {
    render_audio_hal->setMute(mute);
  }
}

void CRedCore::setLoop(int loop_count) {
  std::unique_lock<std::mutex> lck(mLock);
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  player_config->loop = loop_count;
}

int CRedCore::getLoop() {
  std::unique_lock<std::mutex> lck(mLock);
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  return player_config->loop;
}

void CRedCore::notifyListener(uint32_t what, int32_t arg1, int32_t arg2,
                              void *obj1, void *obj2, int obj1_len,
                              int obj2_len) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  if (mNotifyCb) {
    switch (what) {
    case RED_MSG_COMPLETED: {
      mCompleted.store(true);
      PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
      if (player_config->loop != 1 &&
          (!player_config->loop || --player_config->loop)) {
        what = RED_REQ_SEEK;
        arg1 = 0;
        mVideoState->loop_count++;
        mNotifyCb(RED_MSG_SEEK_LOOP_START, mVideoState->loop_count, 0, nullptr,
                  nullptr, 0, 0);
      }
      break;
    }
    case RED_MSG_BUFFERING_START: {
      PerformPause();
      break;
    }
    case RED_MSG_BUFFERING_END: {
      if (!mPausedByClient) {
        PerformStart();
      }
      break;
    }
    case RED_MSG_SEEK_COMPLETE: {
      if (arg1 == mCurSeekPos) {
        mSeeking.store(false);
      }
      PerformFlush();
      break;
    }
    case RED_REQ_START: {
      if (!mPausedByClient) {
        PerformStart();
      }
      return;
    }
    case RED_REQ_INTERNAL_PAUSE: {
      PerformPause();
      return;
    }
    case RED_REQ_INTERNAL_PLAYBACK_RATE: {
      float playback_rate = 1.0 * arg1 / 100;
      setPlaybackRate(playback_rate);
      return;
    }
    default:
      break;
    }
    mNotifyCb(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
  }
}

RED_ERR CRedCore::PerformConfigs() {
  resetConfigs();
  PerformPlayerConfig();
  PerformFormatConfig();
  PerformCodecConfig();
  PerformSwsConfig();
  sp<CRedSourceController> source_controller = mRedSourceController;
  sp<CVideoProcesser> video_processer;
  sp<CAudioProcesser> audio_processer;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    audio_processer = mAudioProcesser;
    render_audio_hal = mRedRenderAudioHal;
  }
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer = mVideoProcesser;
    render_video_hal = mRedRenderVideoHal;
  }

  if (source_controller) {
    source_controller->setConfig(mGeneralConfig);
  }
  if (video_processer) {
    video_processer->setConfig(mGeneralConfig);
  }
  if (audio_processer) {
    audio_processer->setConfig(mGeneralConfig);
  }
  if (render_video_hal) {
    render_video_hal->setConfig(mGeneralConfig);
  }
  if (render_audio_hal) {
    render_audio_hal->setConfig(mGeneralConfig);
  }
  return OK;
}

RED_ERR CRedCore::resetConfigs() {
  mGeneralConfig->playerConfig->reset();
  av_dict_free(&mGeneralConfig->formatConfig);
  av_dict_free(&mGeneralConfig->codecConfig);
  av_dict_free(&mGeneralConfig->swsConfig);
  av_dict_free(&mGeneralConfig->swrConfig);
  return OK;
}

RED_ERR CRedCore::PerformPlayerConfig() {
  sp<RedPlayerConfig> player_config = mGeneralConfig->playerConfig;
  uint8_t *target_obj = reinterpret_cast<uint8_t *>(player_config->get());
  for (size_t i = 0; i < sizeof(allCfg) / sizeof(RedCfg); i++) {
    const char *name = allCfg[i].name;
    int offset = allCfg[i].offset;
    ValType type = allCfg[i].type;
    int64_t default_i64 = allCfg[i].defaultVal.i64;
    const char *default_str = allCfg[i].defaultVal.str;
    void *dst = target_obj + offset;

    if (!name || !dst) {
      continue;
    }

    switch (type) {
    case kTypeInt32:
      *(reinterpret_cast<int32_t *>(dst)) =
          static_cast<int32_t>(mPlayerConfig->GetInt64(name, default_i64));
      break;
    case kTypeInt64:
      *(reinterpret_cast<int64_t *>(dst)) =
          static_cast<int64_t>(mPlayerConfig->GetInt64(name, default_i64));
      break;
    case kTypeString: {
      std::string str = mPlayerConfig->GetString(name, nullptr);
      if (!str.empty()) {
        *(reinterpret_cast<uint8_t **>(dst)) =
            reinterpret_cast<uint8_t *>(strdup(str.c_str()));
      } else if (default_str) {
        *(reinterpret_cast<uint8_t **>(dst)) =
            reinterpret_cast<uint8_t *>(strdup(default_str));
      }
      break;
    }
    default:
      break;
    }
  }
  return OK;
}

RED_ERR CRedCore::PerformFormatConfig() {
  for (size_t i = 0; i < mFormatConfig->CountEntries(); ++i) {
    ValType type = kTypeUnknown;
    const char *name = mFormatConfig->GetEntryNameAt(i, &type);
    if (!name) {
      continue;
    }
    switch (type) {
    case kTypeInt64:
      av_dict_set_int(&mGeneralConfig->formatConfig, name,
                      mFormatConfig->GetInt64(name, 0), 0);
      break;
    case kTypeString:
      av_dict_set(&mGeneralConfig->formatConfig, name,
                  mFormatConfig->GetString(name, nullptr).c_str(), 0);
      break;
    default:
      AV_LOGW_ID(TAG, mID, "[%s][%d] unknown config type %d, name %s\n",
                 __FUNCTION__, __LINE__, type, name);
      break;
    }
  }
  if (!av_dict_get(mGeneralConfig->formatConfig, "scan_all_pmts", NULL,
                   AV_DICT_MATCH_CASE)) {
    av_dict_set(&mGeneralConfig->formatConfig, "scan_all_pmts", "1",
                AV_DICT_DONT_OVERWRITE);
  }
  if (av_stristart(mUrl.c_str(), "rtmp", NULL) ||
      av_stristart(mUrl.c_str(), "rtsp", NULL)) {
    AV_LOGI_ID(TAG, mID, "remove 'timeout' option for rtmp.\n");
    av_dict_set(&mGeneralConfig->formatConfig, "timeout", NULL, 0);
  }

  av_dict_set_intptr(&mGeneralConfig->formatConfig, "redapplication",
                     (uintptr_t)mAppCtx, 0);

  return OK;
}

RED_ERR CRedCore::PerformCodecConfig() {
  for (size_t i = 0; i < mCodecConfig->CountEntries(); ++i) {
    ValType type = kTypeUnknown;
    const char *name = mCodecConfig->GetEntryNameAt(i, &type);
    if (!name) {
      continue;
    }
    switch (type) {
    case kTypeInt64:
      av_dict_set_int(&mGeneralConfig->codecConfig, name,
                      mCodecConfig->GetInt64(name, 0), 0);
      break;
    case kTypeString:
      av_dict_set(&mGeneralConfig->codecConfig, name,
                  mCodecConfig->GetString(name, nullptr).c_str(), 0);
      break;
    default:
      AV_LOGW_ID(TAG, mID, "[%s][%d] unknown config type %d, name %s\n",
                 __FUNCTION__, __LINE__, type, name);
      break;
    }
  }
  return OK;
}

RED_ERR CRedCore::PerformSwsConfig() {
  for (size_t i = 0; i < mSwsConfig->CountEntries(); ++i) {
    ValType type = kTypeUnknown;
    const char *name = mSwsConfig->GetEntryNameAt(i, &type);
    if (!name) {
      continue;
    }
    switch (type) {
    case kTypeInt64:
      av_dict_set_int(&mGeneralConfig->swsConfig, name,
                      mSwsConfig->GetInt64(name, 0), 0);
      break;
    case kTypeString:
      av_dict_set(&mGeneralConfig->swsConfig, name,
                  mSwsConfig->GetString(name, nullptr).c_str(), 0);
      break;
    default:
      AV_LOGW_ID(TAG, mID, "[%s][%d] unknown config type %d, name %s\n",
                 __FUNCTION__, __LINE__, type, name);
      break;
    }
  }

  return OK;
}

RED_ERR CRedCore::PerformStart() {
  if (mAbort) {
    return OK;
  }
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  mVideoState->pause_req = false;
  mVideoState->paused = false;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    render_video_hal = mRedRenderVideoHal;
  }
  if (render_video_hal) {
    render_video_hal->start();
  }
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    render_audio_hal = mRedRenderAudioHal;
  }
  if (render_audio_hal) {
    render_audio_hal->start();
  }

  return OK;
}

RED_ERR CRedCore::PerformPrepare() {
  if (mAbort) {
    return OK;
  }
  auto source_controller = mRedSourceController;
  if (source_controller) {
    source_controller->setPrepareCb(
        std::bind(&CRedCore::PreparedCb, this, std::placeholders::_1));
    return source_controller->prepareAsync();
  }
  return NO_INIT;
}

RED_ERR CRedCore::PerformSeek(int64_t msec) {
  if (mAbort) {
    return OK;
  }
  auto source_controller = mRedSourceController;
  if (!source_controller) {
    return NO_INIT;
  }
  mSeeking.store(true);
  return source_controller->seek(msec);
}
RED_ERR CRedCore::PerformStop() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  sp<CRedSourceController> source_controller = mRedSourceController;
  sp<CVideoProcesser> video_processer;
  sp<CAudioProcesser> audio_processer;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    audio_processer = mAudioProcesser;
    render_audio_hal = mRedRenderAudioHal;
  }
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer = mVideoProcesser;
    render_video_hal = mRedRenderVideoHal;
  }
  if (render_audio_hal) {
    render_audio_hal->stop();
  }
  if (render_video_hal) {
    render_video_hal->stop();
  }
  if (audio_processer) {
    audio_processer->stop();
  }
  if (video_processer) {
    video_processer->stop();
  }
  if (source_controller) {
    source_controller->stop();
  }
  return OK;
}
RED_ERR CRedCore::PerformPause() {
  if (mAbort) {
    return OK;
  }
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  mVideoState->pause_req = true;
  mVideoState->paused = true;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    render_video_hal = mRedRenderVideoHal;
  }
  if (render_video_hal) {
    render_video_hal->pause();
  }
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    render_audio_hal = mRedRenderAudioHal;
  }
  if (render_audio_hal) {
    render_audio_hal->pause();
  }
  return OK;
}

RED_ERR CRedCore::PerformFlush() {
  if (mAbort) {
    return OK;
  }
  sp<CRedSourceController> source_controller = mRedSourceController;
  sp<CVideoProcesser> video_processer;
  sp<CAudioProcesser> audio_processer;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    audio_processer = mAudioProcesser;
    render_audio_hal = mRedRenderAudioHal;
  }
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer = mVideoProcesser;
    render_video_hal = mRedRenderVideoHal;
  }
  if (video_processer) {
    video_processer->resetEof();
  }
  if (audio_processer) {
    audio_processer->resetEof();
  }
  if (render_video_hal) {
    render_video_hal->flush();
  }
  if (render_audio_hal) {
    render_audio_hal->flush();
  }

  return OK;
}

void CRedCore::release() {
  AV_LOGD_ID(TAG, mID, "%s start\n", __func__);
  sp<CRedSourceController> source_controller = mRedSourceController;
  sp<CVideoProcesser> video_processer;
  sp<CAudioProcesser> audio_processer;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    audio_processer = mAudioProcesser;
    render_audio_hal = mRedRenderAudioHal;
  }
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer = mVideoProcesser;
    render_video_hal = mRedRenderVideoHal;
  }

  source_controller->setNotifyCb(nullptr);
  source_controller->setPrepareCb(nullptr);
  source_controller->release();
  if (mAppCtx) {
    free(mAppCtx);
    mAppCtx = nullptr;
  }

  if (video_processer) {
    video_processer->setNotifyCb(nullptr);
    video_processer->release();
  }

  if (audio_processer) {
    audio_processer->setNotifyCb(nullptr);
    audio_processer->release();
  }

  if (render_video_hal) {
    render_video_hal->setNotifyCb(nullptr);
    render_video_hal->release();
  }

  if (render_audio_hal) {
    render_audio_hal->setNotifyCb(nullptr);
    render_audio_hal->release();
  }

  if (mFd >= 0) {
    close(mFd);
    mFd = -1;
  }

  AV_LOGD_ID(TAG, mID, "%s end\n", __func__);
}

RED_ERR CRedCore::setConfig(int type, std::string name, std::string value) {
  std::lock_guard<std::mutex> lck(mLock);
  switch (type) {
  case cfgTypeFormat:
    mFormatConfig->SetString(name.c_str(), value);
    break;
  case cfgTypeCodec:
    mCodecConfig->SetString(name.c_str(), value);
    break;
  case cfgTypeSwr:
    mSwrConfig->SetString(name.c_str(), value);
    break;
  case cfgTypeSws:
    mSwsConfig->SetString(name.c_str(), value);
    break;
  case cfgTypePlayer:
    mPlayerConfig->SetString(name.c_str(), value);
    break;
  default:
    break;
  }
  return OK;
}
RED_ERR CRedCore::setConfig(int type, std::string name, int64_t value) {
  std::lock_guard<std::mutex> lck(mLock);
  switch (type) {
  case cfgTypeFormat:
    mFormatConfig->SetInt64(name.c_str(), value);
    break;
  case cfgTypeCodec:
    mCodecConfig->SetInt64(name.c_str(), value);
    break;
  case cfgTypeSwr:
    mSwrConfig->SetInt64(name.c_str(), value);
    break;
  case cfgTypeSws:
    mSwsConfig->SetInt64(name.c_str(), value);
    break;
  case cfgTypePlayer:
    mPlayerConfig->SetInt64(name.c_str(), value);
    break;
  default:
    break;
  }
  return OK;
}

void CRedCore::setPlaybackRate(float rate) {
  if (std::abs(mVideoState->playback_rate - rate) < FLT_EPSILON) {
    return;
  }
  AV_LOGI_ID(TAG, mID, "rate: %f\n", rate);
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    render_audio_hal = mRedRenderAudioHal;
  }
  if (render_audio_hal) {
    render_audio_hal->setPlaybackRate(rate);
  }
  mVideoState->audio_clock->SetSpeed(rate);
  mVideoState->video_clock->SetSpeed(rate);
  mVideoState->playback_rate = rate;
  checkHighFps();
}

RED_ERR CRedCore::setProp(int property, const int64_t value) { return OK; }

RED_ERR CRedCore::setProp(int property, const float value) {
  std::unique_lock<std::mutex> lck(mLock);
  switch (property) {
  case RED_PROP_FLOAT_PLAYBACK_RATE: {
    setPlaybackRate(value);
    break;
  }
  case RED_PROP_FLOAT_PLAYBACK_VOLUME: {
    setVolume(value, value);
    break;
  }
  default:
    break;
  }
  return OK;
}

int64_t CRedCore::getProp(int property, const int64_t default_value) {
  switch (property) {
  case RED_PROP_INT64_SELECTED_VIDEO_STREAM:
    if (mMetaData) {
      return mMetaData->video_index;
    }
    break;
  case RED_PROP_INT64_SELECTED_AUDIO_STREAM:
    if (mMetaData) {
      return mMetaData->audio_index;
    }
    break;
  case RED_PROP_INT64_VIDEO_DECODER:
    return mVideoState->stat.vdec_type;
  case RED_PROP_INT64_AUDIO_DECODER: {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    auto audio_processer = mAudioProcesser;
    if (audio_processer) {
      return RED_PROPV_DECODER_AVCODEC;
    }
    break;
  }
  case RED_PROP_INT64_VIDEO_CACHED_DURATION:
    return mVideoState->stat.video_cache.duration;
  case RED_PROP_INT64_AUDIO_CACHED_DURATION:
    return mVideoState->stat.audio_cache.duration;
  case RED_PROP_INT64_VIDEO_CACHED_BYTES:
    return mVideoState->stat.video_cache.bytes;
  case RED_PROP_INT64_AUDIO_CACHED_BYTES:
    return mVideoState->stat.audio_cache.bytes;
  case RED_PROP_INT64_VIDEO_CACHED_PACKETS:
    return mVideoState->stat.video_cache.packets;
  case RED_PROP_INT64_AUDIO_CACHED_PACKETS:
    return mVideoState->stat.audio_cache.packets;
  case RED_PROP_INT64_BIT_RATE:
    return mVideoState->stat.bit_rate;
  case RED_PROP_INT64_TCP_SPEED:
    return mVideoState->stat.tcp_read_sampler.getSpeed();
  case RED_PROP_INT64_LAST_TCP_SPEED:
    return mVideoState->stat.tcp_read_sampler.getLastSpeed();
  case RED_PROP_INT64_LATEST_SEEK_LOAD_DURATION:
    return mVideoState->stat.latest_seek_load_duration;
  case RED_PROP_INT64_TRAFFIC_STATISTIC_BYTE_COUNT:
    return mVideoState->stat.byte_count;
  case RED_PROP_INT64_TRAFFIC_STATISTIC_BYTE_INET:
    return mVideoState->stat.byte_count_inet;
  case RED_PROP_INT64_TRAFFIC_STATISTIC_BYTE_INET6:
    return mVideoState->stat.byte_count_inet6;
  case RED_PROP_INT64_CACHED_STATISTIC_BYTE_COUNT:
    return mVideoState->stat.cached_size;
  case RED_PROP_INT64_CACHED_STATISTIC_REAL_BYTE_COUNT:
    return mVideoState->stat.real_cached_size;
  case RED_PROP_INT64_CACHE_STATISTIC_PHYSICAL_POS:
    return mVideoState->stat.cache_physical_pos;
  case RED_PROP_INT64_CACHE_STATISTIC_FILE_FORWARDS:
    return mVideoState->stat.cache_file_forwards;
  case RED_PROP_INT64_CACHE_STATISTIC_FILE_POS:
    return mVideoState->stat.cache_file_pos;
  case RED_PROP_INT64_CACHE_STATISTIC_COUNT_BYTES:
    return mVideoState->stat.cache_count_bytes;
  case RED_PROP_INT64_LOGICAL_FILE_SIZE:
    return mVideoState->stat.logical_file_size;
  case RED_PROP_INT64_MAX_BUFFER_SIZE:
    return mGeneralConfig->playerConfig->get()->dcc.max_buffer_size;
  case RED_PROP_INT64_VIDEO_PIXEL_FORMAT:
    return mVideoState->stat.pixel_format;
  default:
    break;
  }
  return default_value;
}

float CRedCore::getProp(int property, const float default_value) {
  switch (property) {
  case RED_PROP_FLOAT_VIDEO_DECODE_FRAMES_PER_SECOND:
    return mVideoState->stat.vdps;
  case RED_PROP_FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND:
    return mVideoState->stat.vfps;
  case RED_PROP_FLOAT_AVDELAY:
    return mVideoState->stat.avdelay;
  case RED_PROP_FLOAT_AVDIFF:
    return mVideoState->stat.avdiff;
  case RED_PROP_FLOAT_DROP_PACKET_RATE_BEFORE_DECODE:
    return mVideoState->stat.total_packet_count == 0
               ? default_value
               : (1.f * mVideoState->stat.drop_packet_count /
                  mVideoState->stat.total_packet_count);
  case RED_PROP_FLOAT_DROP_FRAME_RATE:
    return mVideoState->stat.drop_frame_rate;
  case RED_PROP_FLOAT_PLAYBACK_RATE:
    return mVideoState->playback_rate;
  case RED_PROP_FLOAT_PLAYBACK_VOLUME:
    return mVideoState->volume;
  case RED_PROP_FLOAT_VIDEO_FILE_FRAME_RATE:
    if (mMetaData && mMetaData->video_index >= 0) {
      TrackInfo track_info = mMetaData->track_info[mMetaData->video_index];
      if (track_info.fps_den > 0) {
        return static_cast<float>(track_info.fps_num) / track_info.fps_den;
      }
    }
    break;
  default:
    break;
  }
  return default_value;
}

void CRedCore::setNotifyCb(NotifyCallback cb) {
  std::unique_lock<std::mutex> lck(mNotifyCbLock);
  mNotifyCb = cb;
}

void *CRedCore::setInjectOpaque(void *opaque) {
  void *prev_weak_thiz = mInjectOpaque.load();
  mInjectOpaque.store(opaque);
  if (!opaque) {
    return prev_weak_thiz;
  }

  memset(mAppCtx, 0, sizeof(RedApplicationContext));
  mAppCtx->func_on_app_event = app_func_event;
  mAppCtx->opaque = reinterpret_cast<void *>(this);
  return prev_weak_thiz;
}

void *CRedCore::getInjectOpaque() { return mInjectOpaque.load(); }

sp<RedDict> CRedCore::getConfig(int type) {
  std::unique_lock<std::mutex> lck(mLock);
  sp<RedDict> ret;
  switch (type) {
  case cfgTypeFormat:
    return mFormatConfig;
  case cfgTypeCodec:
    return mCodecConfig;
  case cfgTypeSwr:
    return mSwrConfig;
  case cfgTypeSws:
    return mSwsConfig;
  case cfgTypePlayer:
    return mPlayerConfig;
  default:
    break;
  }
  return ret;
}

RED_ERR CRedCore::getVideoCodecInfo(std::string &codec_info) {
  codec_info = mVideoState->video_codec_info;
  return OK;
}

RED_ERR CRedCore::getAudioCodecInfo(std::string &codec_info) {
  codec_info = mVideoState->audio_codec_info;
  return OK;
}

void CRedCore::checkHighFps() {
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  int video_index = -1;
  if (mMetaData) {
    video_index = mMetaData->video_index;
  }
  if (!player_config || video_index < 0) {
    return;
  }
  TrackInfo track_info = mMetaData->track_info[video_index];
  if (player_config->max_fps >= 0) {
    if (track_info.fps_den > 0 && track_info.fps_num > 0) {
      double fps = track_info.fps_num / static_cast<double>(track_info.fps_den);
      if (std::abs(mVideoState->playback_rate - 1.0f) > FLT_EPSILON) {
        fps = fps * mVideoState->playback_rate;
      }
      if (fps > player_config->max_fps) {
        mVideoState->is_video_high_fps = true;
      } else {
        mVideoState->is_video_high_fps = false;
      }
    }
  }
  if (mVideoState->is_video_high_fps) {
    mVideoState->skip_frame =
        std::max(mVideoState->skip_frame, static_cast<int>(DISCARD_NONREF));
  } else {
    mVideoState->skip_frame =
        std::min(mVideoState->skip_frame, static_cast<int>(DISCARD_DEFAULT));
  }
}

void CRedCore::PreparedCb(sp<MetaData> &metadata) {
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (!player_config->start_on_prepared) {
    PerformPause();
  }
  if (metadata) {
    mMetaData = metadata;
    mVideoState->audio_stream_index = mMetaData->audio_index;
    mVideoState->video_stream_index = mMetaData->video_index;
    mVideoState->stat.bit_rate = mMetaData->bit_rate;
    PrepareStream(metadata);
    if (metadata->video_index >= 0) {
      int video_idx = metadata->video_index;
      notifyListener(RED_MSG_VIDEO_SIZE_CHANGED,
                     metadata->track_info[video_idx].width,
                     metadata->track_info[video_idx].height);
      notifyListener(RED_MSG_SAR_CHANGED,
                     metadata->track_info[video_idx].sar_num,
                     metadata->track_info[video_idx].sar_den);
    }
    if (player_config->start_on_prepared) {
      PerformStart();
    }
    dumpFormatInfo();
  } else {
    AV_LOGE_ID(TAG, mID, "Fail to find stream info!\n");
    notifyListener(RED_MSG_ERROR, ERROR_STREAM_OPEN);
  }
  notifyListener(RED_MSG_PREPARED);

  if (player_config->seek_at_start > 0) {
    seekTo(player_config->seek_at_start, true);
  }
}

void CRedCore::PrepareStream(sp<MetaData> &metadata) {
  RED_ERR ret = OK;
  PlayerConfig *player_config = mGeneralConfig->playerConfig->get();
  if (mAbort || !metadata || !player_config)
    return;
  if (metadata->audio_index < 0 && metadata->video_index < 0) {
    AV_LOGE_ID(TAG, mID, "No available stream!\n");
    notifyListener(RED_MSG_ERROR, ERROR_FIND_STREAM_INFO);
    return;
  }
  sp<CRedSourceController> source_controller = mRedSourceController;
  sp<CVideoProcesser> video_processer;
  sp<CAudioProcesser> audio_processer;
  sp<CRedRenderVideoHal> render_video_hal;
  sp<CRedRenderAudioHal> render_audio_hal;
  {
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    audio_processer = mAudioProcesser;
    render_audio_hal = mRedRenderAudioHal;
  }
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer = mVideoProcesser;
    render_video_hal = mRedRenderVideoHal;
  }
  if (metadata->audio_index >= 0 && !player_config->audio_disable &&
      audio_processer) {
    ret = audio_processer->Prepare(metadata);
    if (ret == OK && render_audio_hal) {
      ret = render_audio_hal->Prepare(metadata);
      if (ret == OK) {
        mVideoState->av_sync_type = CLOCK_AUDIO;
        AV_LOGI_ID(TAG, mID, "prepare audio stream success\n");
      }
    }
    if (ret != OK) {
      AV_LOGI_ID(TAG, mID, "%s audio failed\n", __func__);
      metadata->audio_index = -1;
      if (source_controller) {
        source_controller->pktQueueAbort(TYPE_AUDIO);
      }
      audio_processer->release();
      render_audio_hal->release();
      std::unique_lock<std::mutex> lck(mAudioStreamLock);
      audio_processer.reset();
      mAudioProcesser.reset();
      render_audio_hal.reset();
      mRedRenderAudioHal.reset();
    }
  } else {
    AV_LOGD_ID(TAG, mID, "Stream not have Audio\n");
    std::unique_lock<std::mutex> lck(mAudioStreamLock);
    audio_processer.reset();
    mAudioProcesser.reset();
    render_audio_hal.reset();
    mRedRenderAudioHal.reset();
  }

  if (metadata->video_index >= 0 && !player_config->video_disable &&
      video_processer) {
    TrackInfo track_info = metadata->track_info[metadata->video_index];
    parseVideoExtraData(track_info);
    ret = video_processer->Prepare(metadata);
    if (ret == OK && render_video_hal) {
      ret = render_video_hal->Prepare(metadata);
      if (ret == OK) {
        if (mVideoState->av_sync_type != CLOCK_AUDIO) {
          mVideoState->av_sync_type = CLOCK_VIDEO;
        }
        checkHighFps();
        AV_LOGI_ID(TAG, mID, "prepare video stream success\n");
      }
    }
    if (ret != OK) {
      AV_LOGI_ID(TAG, mID, "%s video failed\n", __func__);
      metadata->video_index = -1;
      if (source_controller) {
        source_controller->pktQueueAbort(TYPE_VIDEO);
      }
      video_processer->release();
      render_video_hal->release();
      std::unique_lock<std::mutex> lck(mVideoStreamLock);
      video_processer.reset();
      mVideoProcesser.reset();
      render_video_hal.reset();
      mRedRenderVideoHal.reset();
    }
  } else {
    AV_LOGI_ID(TAG, mID, "Stream not have Video\n");
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer.reset();
    mVideoProcesser.reset();
    render_video_hal.reset();
    mRedRenderVideoHal.reset();
  }
  notifyListener(RED_MSG_COMPONENT_OPEN);
}

void CRedCore::parseVideoExtraData(TrackInfo &info) {
  if (info.codec_id == AV_CODEC_ID_H264 && info.extra_data_size > 3 &&
      info.extra_data && info.extra_data[0]) {
    mVideoState->nal_length_size = (info.extra_data[4] & 0x03) + 1;
  } else if (info.codec_id == AV_CODEC_ID_H265 && info.extra_data &&
             (info.extra_data[0] || info.extra_data[1] ||
              info.extra_data[2] > 1)) {
    if (info.extra_data_size > 21) {
      mVideoState->nal_length_size = (info.extra_data[21] & 0x03) + 1;
    }
  }
}

void CRedCore::dumpFormatInfo() {
  if (mMetaData) {
    AV_LOGI_ID(TAG, mID, "========== Video format info start ==========\n");
    if (mMetaData->video_index >= 0) {
      TrackInfo video_track = mMetaData->track_info[mMetaData->video_index];
      AV_LOGI_ID(TAG, mID,
                 "Video: %s, %s(%s, %s), %dx%d, SAR: %d:%d, %d kb/s, FPS: "
                 "%d:%d, TBR: %d:%d\n",
                 avcodec_get_name(static_cast<AVCodecID>(video_track.codec_id)),
                 av_get_pix_fmt_name(
                     static_cast<AVPixelFormat>(video_track.pixel_format)),
                 av_color_range_name(
                     static_cast<AVColorRange>(video_track.color_range)),
                 av_color_space_name(
                     static_cast<AVColorSpace>(video_track.color_space)),
                 video_track.width, video_track.height, video_track.sar_num,
                 video_track.sar_den,
                 static_cast<int>(video_track.bit_rate / 1000),
                 video_track.fps_num, video_track.fps_den, video_track.tbr_num,
                 video_track.tbr_den);
    }
    if (mMetaData->audio_index >= 0) {
      TrackInfo audio_track = mMetaData->track_info[mMetaData->audio_index];
      char buf[256] = {0};
      av_get_channel_layout_string(buf, sizeof(buf), audio_track.channels,
                                   audio_track.channel_layout);
      AV_LOGI_ID(TAG, mID, "Audio: %s, %d Hz, %s, %s, %d kb/s\n",
                 avcodec_get_name(static_cast<AVCodecID>(audio_track.codec_id)),
                 audio_track.sample_rate, buf,
                 av_get_sample_fmt_name(
                     static_cast<AVSampleFormat>(audio_track.sample_fmt)),
                 static_cast<int>(audio_track.bit_rate / 1000));
    }
    AV_LOGI_ID(TAG, mID, "========== Video format info end ==========\n");
  }
}

#if defined(__ANDROID__) || defined(__HARMONY__)
RED_ERR CRedCore::setVideoSurface(const sp<RedNativeWindow> &surface) {
  sp<CVideoProcesser> video_processer;
  sp<CRedRenderVideoHal> render_video_hal;
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    video_processer = mVideoProcesser;
    render_video_hal = mRedRenderVideoHal;
  }

  if (!video_processer || !render_video_hal) {
    return NO_INIT;
  }

  {
    std::unique_lock<std::mutex> lck(mSurfaceLock);
    mNativeWindow = surface;
  }

  render_video_hal->setVideoSurface(surface);

  if (video_processer->setVideoSurface(surface) == ME_STATE_ERROR) {
    int64_t pos = 0;
    getCurrentPosition(pos);
    if (surface && surface->get() && pos >= 0 &&
        mVideoState->stat.vdec_type == RED_PROPV_DECODER_MEDIACODEC) {
      seekTo(pos);
    }
  }
  return OK;
}

void CRedCore::getWidth(int32_t &width) {
  width = 0;
  if (!mMetaData) {
    return;
  }

  int video_index = mMetaData->video_index;
  if (video_index >= 0) {
    width = mMetaData->track_info[video_index].width;
  }
  return;
}

void CRedCore::getHeight(int32_t &height) {
  height = 0;
  if (!mMetaData) {
    return;
  }

  int video_index = mMetaData->video_index;
  if (video_index >= 0) {
    height = mMetaData->track_info[video_index].height;
  }
  return;
}
#endif

#if defined(__APPLE__)
UIView *CRedCore::initWithFrame(int type, CGRect cgrect) {
  sp<CRedRenderVideoHal> render_video_hal;
  {
    std::unique_lock<std::mutex> lck(mVideoStreamLock);
    render_video_hal = mRedRenderVideoHal;
  }
  if (!render_video_hal) {
    return nullptr;
  }
  return render_video_hal->initWithFrame(type, cgrect);
}
#endif

REDPLAYER_NS_END;
