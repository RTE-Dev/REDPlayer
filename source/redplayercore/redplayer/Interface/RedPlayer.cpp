#include "RedPlayer.h"
#include "RedLog.h"
#include "RedMeta.h"
#include "strategy/RedAdaptiveStrategy.h"
#include "wrapper/reddownload_datasource_wrapper.h"
#include <atomic>

#define TAG "RedPlayer"

using redstrategycenter::strategy::RedAdaptiveStrategy;

REDPLAYER_NS_BEGIN;

static bool g_ffmpeg_global_inited{false};
static int g_log_callback_level{RED_LOG_DEBUG};
static LogCallback g_log_callback;
static std::mutex *g_log_mutex = new std::mutex();
static std::atomic<int> g_alive_player_count{0};

static inline int logLevelAppToAv(int level) {
  int av_level = AV_LOG_TRACE;
  if (level >= RED_LOG_SILENT)
    av_level = AV_LOG_QUIET;
  else if (level >= RED_LOG_FATAL)
    av_level = AV_LOG_FATAL;
  else if (level >= RED_LOG_ERROR)
    av_level = AV_LOG_ERROR;
  else if (level >= RED_LOG_WARN)
    av_level = AV_LOG_WARNING;
  else if (level >= RED_LOG_INFO)
    av_level = AV_LOG_INFO;
  else if (level >= RED_LOG_DEBUG)
    av_level = AV_LOG_DEBUG;
  else if (level >= RED_LOG_VERBOSE)
    av_level = AV_LOG_TRACE;
  else if (level >= RED_LOG_DEFAULT)
    av_level = AV_LOG_TRACE;
  else if (level >= RED_LOG_UNKNOWN)
    av_level = AV_LOG_TRACE;
  else
    av_level = AV_LOG_TRACE;
  return av_level;
}

static inline int logLevelAvToApp(int red_level) {
  int app_level = RED_LOG_VERBOSE;
  if (red_level <= AV_LOG_QUIET)
    app_level = RED_LOG_SILENT;
  else if (red_level <= AV_LOG_FATAL)
    app_level = RED_LOG_FATAL;
  else if (red_level <= AV_LOG_ERROR)
    app_level = RED_LOG_ERROR;
  else if (red_level <= AV_LOG_WARNING)
    app_level = RED_LOG_WARN;
  else if (red_level <= AV_LOG_INFO)
    app_level = RED_LOG_INFO;
  else if (red_level <= AV_LOG_DEBUG)
    app_level = RED_LOG_DEBUG;
  else if (red_level <= AV_LOG_TRACE)
    app_level = RED_LOG_VERBOSE;
  else
    app_level = RED_LOG_VERBOSE;
  return app_level;
}

static void logCallbackReport(void *ptr, int level, const char *fmt,
                              va_list vl) {
  std::unique_lock<std::mutex> lck(*g_log_mutex);

  if (!g_log_callback) {
    return;
  }

  int app_level = logLevelAvToApp(level);
  if (app_level < g_log_callback_level) {
    return;
  }

  va_list vl2;
  char line[1024 + 256];
  static int print_prefix = 1;

  va_copy(vl2, vl);
  av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
  va_end(vl2);

  g_log_callback(app_level, TAG, line);
}

static void logCallbackWrapper(void *ptr, int level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logCallbackReport(ptr, level, fmt, args);
  va_end(args);
}

static void redLogCallback(void *ptr, int level, const char *buf) {
  if (buf) {
    logCallbackWrapper(nullptr, level, "%s", buf);
  }
}

void globalInit() {
  if (g_ffmpeg_global_inited) {
    return;
  }
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)
  avcodec_register_all();
#endif
#if CONFIG_AVDEVICE
  avdevice_register_all();
#endif
#if CONFIG_AVFILTER
  avfilter_register_all();
#endif
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
  av_register_all();
#endif

  redav_register_all();

  avformat_network_init();
  RedLogSetLevel(AV_LEVEL_DEBUG);
  av_log_set_callback(logCallbackReport);
  g_ffmpeg_global_inited = true;
}

void globalUninit() {
  if (!g_ffmpeg_global_inited) {
    return;
  }
  avformat_network_deinit();
  g_ffmpeg_global_inited = false;
}

void setLogLevel(int level) {
  std::unique_lock<std::mutex> lck(*g_log_mutex);
  int av_log_level = logLevelAppToAv(level);
  av_log_set_level(av_log_level);
}

void setLogCallbackLevel(int level) {
  std::unique_lock<std::mutex> lck(*g_log_mutex);
  g_log_callback_level = level;
}

void setLogCallback(LogCallback cb) {
  std::unique_lock<std::mutex> lck(*g_log_mutex);
  av_log_set_callback(logCallbackReport);
  RedLogSetCallback(redLogCallback, nullptr);
  g_log_callback = cb;
}

CRedPlayer::CRedPlayer(int id, MsgCallback msg_cb) : mID(id), mMsgCb(msg_cb) {
  g_alive_player_count++;
  AV_LOGD_ID(TAG, mID, "%s,g_alive_player_count:%d\n", __func__,
             g_alive_player_count.load());
}

CRedPlayer::~CRedPlayer() {
  mRedCore.reset();
  g_alive_player_count--;
  AV_LOGD_ID(TAG, mID, "%s end, g_alive_player_count:%d\n", __func__,
             g_alive_player_count.load());
}

sp<CRedPlayer> CRedPlayer::Create(int id, MsgCallback msg_cb) {
  sp<CRedPlayer> ret = std::shared_ptr<CRedPlayer>(new CRedPlayer(id, msg_cb));
  if (!ret) {
    return ret;
  }
  if (ret->init() != OK) {
    ret.reset();
  }
  return ret;
}

RED_ERR CRedPlayer::init() {
  try {
    mRedCore = std::make_shared<CRedCore>(
        mID, std::bind(&CRedPlayer::notifyListener, this, std::placeholders::_1,
                       std::placeholders::_2, std::placeholders::_3,
                       std::placeholders::_4, std::placeholders::_5,
                       std::placeholders::_6, std::placeholders::_7));
  } catch (const std::bad_alloc &e) {
    AV_LOGE(TAG, "[%s:%d] Exception caught: %s!\n", __FUNCTION__, __LINE__,
            e.what());
    return NO_MEMORY;
  } catch (...) {
    AV_LOGE(TAG, "[%s:%d] Exception caught!\n", __FUNCTION__, __LINE__);
    return NO_MEMORY;
  }

  if (mRedCore->init() != OK) {
    return NO_MEMORY;
  }
  mMsgQueue.flush();
  return OK;
}

int CRedPlayer::id() const { return mID; }

RED_ERR CRedPlayer::setDataSource(std::string url) {
  AV_LOGD_ID(TAG, mID, "%s %s\n", __func__, url.c_str());
  std::unique_lock<std::mutex> lck(mLock);
  if (url.empty()) {
    return ME_ERROR;
  }
  if (mPlayerState != MP_STATE_IDLE) {
    return ME_STATE_ERROR;
  }
  lck.unlock();
  checkInputIsJson(url);
  mRedCore->setDataSource(mDataSource);
  lck.lock();
  changeState(MP_STATE_INITIALIZED);
  return OK;
}
RED_ERR CRedPlayer::setDataSourceFd(int64_t fd) {
  AV_LOGD_ID(TAG, mID, "%s %" PRId64 "\n", __func__, fd);
  std::unique_lock<std::mutex> lck(mLock);
  if (mPlayerState != MP_STATE_IDLE) {
    return ME_STATE_ERROR;
  }
  lck.unlock();
  mRedCore->setDataSourceFd(fd);
  lck.lock();
  changeState(MP_STATE_INITIALIZED);
  return OK;
}
RED_ERR CRedPlayer::prepareAsync() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  std::unique_lock<std::mutex> lck(mLock);
  if (mPlayerState != MP_STATE_INITIALIZED &&
      mPlayerState != MP_STATE_STOPPED) {
    return ME_STATE_ERROR;
  }
  changeState(MP_STATE_ASYNC_PREPARING);
  mMsgQueue.start();
  RED_ERR ret = OK;

  try {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      return ret;
    }
    mMsgThread = std::thread(
        [](CRedPlayer *mp) {
          if (mp) {
            mp->mMsgCb(mp);
          }
        },
        this);
  } catch (const std::system_error &e) {
    AV_LOGE_ID(TAG, mID, "[%s:%d] Exception caught: %s!\n", __FUNCTION__,
               __LINE__, e.what());
    ret = ME_ERROR;
    goto end;
  } catch (...) {
    AV_LOGE_ID(TAG, mID, "[%s:%d] Exception caught!\n", __FUNCTION__, __LINE__);
    ret = ME_ERROR;
    goto end;
  }
  lck.unlock();
  ret = mRedCore->prepareAsync();
  lck.lock();

end:
  if (ret != OK) {
    changeState(MP_STATE_ERROR);
  }
  return ret;
}
RED_ERR CRedPlayer::start() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  {
    Autolock lck(mLock);
    if (!checkStateStart()) {
      return ME_STATE_ERROR;
    }
  }
  mMsgQueue.remove(RED_REQ_START);
  mMsgQueue.remove(RED_REQ_PAUSE);
  notifyListener(RED_REQ_START);
  return OK;
}
RED_ERR CRedPlayer::pause() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  {
    Autolock lck(mLock);
    if (!checkStatePause()) {
      return ME_STATE_ERROR;
    }
  }
  mMsgQueue.remove(RED_REQ_START);
  mMsgQueue.remove(RED_REQ_PAUSE);
  notifyListener(RED_REQ_PAUSE);
  return OK;
}
RED_ERR CRedPlayer::seekTo(int64_t msec) {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  {
    Autolock lck(mLock);
    if (!checkStateSeek()) {
      return ME_STATE_ERROR;
    }
  }
  mCurSeekPos.store(msec);
  mSeeking.store(true);
  mMsgQueue.remove(RED_REQ_SEEK);
  notifyListener(RED_REQ_SEEK, msec);
  return OK;
}
RED_ERR CRedPlayer::stop() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  std::unique_lock<std::mutex> lck(mLock);
  if (mPlayerState != MP_STATE_ASYNC_PREPARING &&
      mPlayerState != MP_STATE_PREPARED && mPlayerState != MP_STATE_STARTED &&
      mPlayerState != MP_STATE_PAUSED && mPlayerState != MP_STATE_COMPLETED &&
      mPlayerState != MP_STATE_STOPPED) {
    return ME_STATE_ERROR;
  }
  mMsgQueue.remove(RED_REQ_START);
  mMsgQueue.remove(RED_REQ_PAUSE);
  mMsgQueue.abort();
  RED_ERR ret = OK;
  lck.unlock();
  ret = mRedCore->stop();
  lck.lock();
  if (OK == ret) {
    changeState(MP_STATE_STOPPED);
  }
  return ret;
}

void CRedPlayer::release() {
  AV_LOGD_ID(TAG, mID, "%s\n", __func__);
  std::unique_lock<std::mutex> lck(mLock);
  changeState(MP_STATE_INTERRUPT);
  lck.unlock();
  mMsgQueue.abort();
  mMsgQueue.flush();
  mRedCore->setNotifyCb(nullptr);
  mRedCore->release();
  {
    std::unique_lock<std::mutex> lck(mThreadLock);
    if (mReleased) {
      AV_LOGD_ID(TAG, mID, "%s already released, just return.\n", __func__);
      return;
    }
    mReleased = true;
  }
  if (mMsgThread.joinable()) {
    mMsgThread.join();
  }
  AV_LOGD_ID(TAG, mID, "%s end\n", __func__);
}

RED_ERR CRedPlayer::getCurrentPosition(int64_t &pos_ms) {
  if (mSeeking.load()) {
    pos_ms = mCurSeekPos;
    return OK;
  }
  if (!mRedCore) {
    pos_ms = 0;
    return NO_INIT;
  }
  return mRedCore->getCurrentPosition(pos_ms);
}

RED_ERR CRedPlayer::getDuration(int64_t &dur_ms) {
  return mRedCore->getDuration(dur_ms);
}

RED_ERR CRedPlayer::getPlayableDuration(int64_t &dur_ms) {
  dur_ms = mRedCore->mVideoState->playable_duration_ms;
  return OK;
}

bool CRedPlayer::isPlaying() { return (mPlayerState == MP_STATE_STARTED); }

bool CRedPlayer::isRendering() {
  return (mRedCore->mVideoState->first_video_frame_rendered);
}

void CRedPlayer::setVolume(const float left_volume, const float right_volume) {
  mRedCore->setVolume(left_volume, right_volume);
}

void CRedPlayer::setMute(bool mute) { mRedCore->setMute(mute); }

void CRedPlayer::setLoop(int loop_count) {
  mRedCore->setLoop(loop_count);
  mRedCore->setConfig(cfgTypePlayer, "loop", (int64_t)loop_count);
}

int CRedPlayer::getLoop() { return mRedCore->getLoop(); }

void CRedPlayer::notifyListener(uint32_t what, int32_t arg1, int32_t arg2,
                                void *obj1, void *obj2, int obj1_len,
                                int obj2_len) {
  mMsgQueue.put(what, arg1, arg2, obj1, obj2, obj1_len, obj2_len);
}

RED_ERR CRedPlayer::setConfig(int type, std::string name, std::string value) {
  return mRedCore->setConfig(type, name, value);
}

RED_ERR CRedPlayer::setConfig(int type, std::string name, int64_t value) {
  return mRedCore->setConfig(type, name, value);
}

RED_ERR CRedPlayer::setProp(int property, const int64_t value) {
  return mRedCore->setProp(property, value);
}

RED_ERR CRedPlayer::setProp(int property, const float value) {
  return mRedCore->setProp(property, value);
}

int64_t CRedPlayer::getProp(int property, const int64_t default_value) {
  return mRedCore->getProp(property, default_value);
}

float CRedPlayer::getProp(int property, const float default_value) {
  return mRedCore->getProp(property, default_value);
}

RED_ERR CRedPlayer::getVideoCodecInfo(std::string &codec_info) {
  return mRedCore->getVideoCodecInfo(codec_info);
}

RED_ERR CRedPlayer::getAudioCodecInfo(std::string &codec_info) {
  return mRedCore->getAudioCodecInfo(codec_info);
}

RED_ERR CRedPlayer::getPlayUrl(std::string &url) {
  if (playUrlReady.load()) {
    url = mRedCore->mVideoState->play_url;
  }
  return OK;
}

int CRedPlayer::getPlayerState() {
  Autolock lck(mLock);
  return static_cast<int>(mPlayerState);
}

#if defined(__ANDROID__)
RED_ERR CRedPlayer::setVideoSurface(JNIEnv *env, jobject jsurface) {
  RED_ERR ret = OK;
  std::unique_lock<std::mutex> lck(mSurfaceLock);
  jobject prev_surface = mSurface;
  if (prev_surface == jsurface ||
      (prev_surface && jsurface && env->IsSameObject(prev_surface, jsurface))) {
    AV_LOGI_ID(TAG, mID, "%s same surface\n", __func__);
    return ret;
  }

  if (jsurface) {
    mSurface = JniNewGlobalRefCatchAll(env, jsurface);
  } else {
    mSurface = nullptr;
  }
  lck.unlock();

  ANativeWindow *nativeWindow = nullptr;
  if (jsurface) {
    nativeWindow = ANativeWindow_fromSurface(env, jsurface);
    if (!nativeWindow) {
      AV_LOGW_ID(TAG, mID, "%s %p null nativeWindow\n", __func__, jsurface);
    }
  }

  auto redNativeWindow = std::make_shared<RedNativeWindow>(nativeWindow);
  ret = mRedCore->setVideoSurface(redNativeWindow);

  if (prev_surface) {
    JniDeleteGlobalRefP(env, &prev_surface);
  }

  if (nativeWindow) {
    ANativeWindow_release(nativeWindow);
  }
  return ret;
}
#endif

#ifdef __HARMONY__
RED_ERR CRedPlayer::setVideoSurface(OHNativeWindow *native_window) {
  RED_ERR ret = OK;
  bool ignore = false;
  std::unique_lock<std::mutex> lck(mSurfaceLock);
  OHNativeWindow *prev_window = mWindow;
  lck.unlock();

  if (prev_window == native_window) {
    ignore = true;
  }

  if (ignore) {
    AV_LOGI_ID(TAG, mID, "%s ignore same window\n", __func__);
    return ret;
  }

  sp<RedNativeWindow> native_window_p =
      std::make_shared<RedNativeWindow>(native_window);

  ret = mRedCore->setVideoSurface(native_window_p);
  return ret;
}

void CRedPlayer::getWidth(int32_t &width) {
  if (!mRedCore) {
    width = 0;
    return;
  }
  return mRedCore->getWidth(width);
}

void CRedPlayer::getHeight(int32_t &height) {
  if (!mRedCore) {
    height = 0;
    return;
  }
  return mRedCore->getHeight(height);
}

#endif

#if defined(__APPLE__)
UIView *CRedPlayer::initWithFrame(int type, CGRect cgrect) {
  Autolock lck(mLock);
  return mRedCore->initWithFrame(type, cgrect);
}
#endif

void *CRedPlayer::setInjectOpaque(void *opaque) {
  void *prev_weak_thiz = nullptr;
  prev_weak_thiz = mRedCore->setInjectOpaque(opaque);
  return prev_weak_thiz;
}

void *CRedPlayer::setWeakThiz(void *weak_thiz) {
  void *prevWeakThiz = mWeakThiz.load();
  mWeakThiz.store(weak_thiz);
  return prevWeakThiz;
}

void *CRedPlayer::getWeakThiz() { return mWeakThiz.load(); }

sp<Message> CRedPlayer::getMessage(bool block) {
  while (1) {
    if (mPlayerState == MP_STATE_INTERRUPT) {
      AV_LOGD_ID(TAG, mID, "%s: playerstate interrupt, return null\n",
                 __func__);
      return nullptr;
    }
    bool waitNext = false;
    sp<Message> msg = mMsgQueue.get(block);
    if (!msg) {
      AV_LOGD_ID(TAG, mID, "%s: null msg\n", __func__);
      return nullptr;
    }

    switch (msg->mWhat) {
    case RED_MSG_PREPARED: {
      std::unique_lock<std::mutex> lck(mLock);
      if (mPlayerState == MP_STATE_ASYNC_PREPARING) {
        changeState(MP_STATE_PREPARED);
      } else {
        AV_LOGD_ID(
            TAG, mID,
            "%s: RED_MSG_PREPARED: expecting state: MP_STATE_ASYNC_PREPARING\n",
            __func__);
      }
      lck.unlock();
      sp<RedDict> player_config =
          mRedCore ? mRedCore->getConfig(cfgTypePlayer) : nullptr;
      lck.lock();
      if (!player_config->GetInt64("start-on-prepared", 1)) {
        changeState(MP_STATE_PAUSED);
      } else {
        changeState(MP_STATE_STARTED);
      }
      break;
    }
    case RED_MSG_COMPLETED: {
      {
        std::unique_lock<std::mutex> lck(mLock);
        mRestart = true;
        mRestartFromBeginning = true;
        changeState(MP_STATE_COMPLETED);
      }
      mRedCore->pause();
      break;
    }
    case RED_MSG_SEEK_COMPLETE: {
      mSeeking.store(false);
      mCurSeekPos.store(-1);
      break;
    }
    case RED_REQ_START: {
      waitNext = true;
      RED_ERR ret = OK;
      std::unique_lock<std::mutex> lck(mLock);
      if (checkStateStart() && mRedCore) {
        if (mRestart) {
          if (mRestartFromBeginning) {
            lck.unlock();
            ret = mRedCore->startFrom(0);
          } else {
            lck.unlock();
            ret = mRedCore->start();
          }
          lck.lock();
          if (ret == OK) {
            changeState(MP_STATE_STARTED);
          }
          mRestart = false;
          mRestartFromBeginning = false;
        } else {
          lck.unlock();
          ret = mRedCore->start();
          lck.lock();
          if (ret == OK) {
            changeState(MP_STATE_STARTED);
          }
        }
      }
      break;
    }
    case RED_REQ_PAUSE: {
      waitNext = true;
      std::unique_lock<std::mutex> lck(mLock);
      if (checkStatePause()) {
        lck.unlock();
        RED_ERR pause_ret = mRedCore->pause();
        lck.lock();
        if (pause_ret == OK) {
          changeState(MP_STATE_PAUSED);
        }
      }
      break;
    }
    case RED_REQ_SEEK: {
      waitNext = true;
      std::unique_lock<std::mutex> lck(mLock);
      if (checkStateSeek()) {
        mRestartFromBeginning = false;
        int32_t pos = msg->mArg1;
        lck.unlock();
        if (mRedCore->seekTo(pos) == OK) {
          AV_LOGV_ID(TAG, mID, "%s: RED_REQ_SEEK: seek to %d\n", __func__, pos);
        }
      }
      break;
    }
    }

    if (waitNext) {
      recycleMessage(msg);
      continue;
    }

    return msg;
  }

  return nullptr;
}

RED_ERR CRedPlayer::recycleMessage(sp<Message> &msg) {
  return mMsgQueue.recycle(msg);
}

void CRedPlayer::changeState(PlayerState target_state) {
  mPlayerState = target_state;
  notifyListener(RED_MSG_PLAYBACK_STATE_CHANGED);
}

bool CRedPlayer::checkStateStart() {
  if (mPlayerState != MP_STATE_PREPARED && mPlayerState != MP_STATE_STARTED &&
      mPlayerState != MP_STATE_PAUSED && mPlayerState != MP_STATE_COMPLETED) {
    return false;
  }
  return true;
}

bool CRedPlayer::checkStatePause() {
  if (mPlayerState != MP_STATE_PREPARED && mPlayerState != MP_STATE_STARTED &&
      mPlayerState != MP_STATE_PAUSED && mPlayerState != MP_STATE_COMPLETED) {
    return false;
  }
  return true;
}

bool CRedPlayer::checkStateSeek() {
  if (mPlayerState != MP_STATE_PREPARED && mPlayerState != MP_STATE_STARTED &&
      mPlayerState != MP_STATE_PAUSED && mPlayerState != MP_STATE_COMPLETED) {
    return false;
  }
  return true;
}

void CRedPlayer::checkInputIsJson(const std::string &url) {
  sp<RedDict> player_config = mRedCore->getConfig(cfgTypePlayer);
  if (player_config) {
    int64_t is_input_json = player_config->GetInt64("is-input-json", 0);
    if (is_input_json > 0) {
      std::unique_ptr<RedAdaptiveStrategy> strategy = nullptr;
      strategy = std::make_unique<RedAdaptiveStrategy>(
          redstrategycenter::adaptive::logic::AbstractAdaptationLogic::
              LogicType::Adaptive);
      if (strategy) {
        strategy->setPlaylist(url);
        strategy->setReddownloadUrlCachedFunc(
            reddownload_datasource_wrapper_cache_size);
        sp<RedDict> format_config = mRedCore->getConfig(cfgTypeFormat);
        mDataSource =
            strategy->getInitialUrlList(strategy->getInitialRepresentation());
        mRedCore->mVideoState->play_url =
            strategy->getInitialUrl(strategy->getInitialRepresentation());
      }
    }
  }

  if (mDataSource.empty()) {
    mDataSource = url;
  }

  if (mRedCore->mVideoState->play_url.empty()) {
    mRedCore->mVideoState->play_url = mDataSource;
  }
  playUrlReady.store(true);
}

REDPLAYER_NS_END;
