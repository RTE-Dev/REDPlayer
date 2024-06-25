#pragma once

#include "RedBase.h"
#include "RedCore/RedCore.h"
#include "RedDef.h"
#include "RedError.h"
#include "RedMeta.h"
#include "RedMsg.h"
#include "RedProp.h"
#include "base/RedConfig.h"
#include "base/RedMsgQueue.h"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
extern "C" {
#include "RedCore/module/sourcer/format/RedFormat.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}
#if defined(__ANDROID__)
#include "jni/Util.h"
#include <android/native_window_jni.h>
#include <jni.h>
#endif

REDPLAYER_NS_BEGIN;

void globalInit();
void globalUninit();
void setLogLevel(int level);
void setLogCallback(LogCallback cb);
void setLogCallbackLevel(int level);

class CRedPlayer {
  using MsgCallback = std::function<RED_ERR(CRedPlayer *)>;

public:
  ~CRedPlayer();

  static sp<CRedPlayer> Create(int id, MsgCallback msg_cb);
  int id() const;

  RED_ERR setDataSource(std::string url);
  RED_ERR setDataSourceFd(int64_t fd);
  RED_ERR prepareAsync();
  RED_ERR start();
  RED_ERR pause();
  RED_ERR seekTo(int64_t msec);
  RED_ERR stop();
  void release();
  RED_ERR getCurrentPosition(int64_t &pos_ms);
  RED_ERR getDuration(int64_t &dur_ms);
  RED_ERR getPlayableDuration(int64_t &dur_ms);
  bool isPlaying();
  bool isRendering();
  void setVolume(const float left_volume, const float right_volume);
  void setMute(bool mute);
  void setLoop(int loop_count);
  int getLoop();

  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);

  RED_ERR setConfig(int type, std::string name, std::string value);
  RED_ERR setConfig(int type, std::string name, int64_t value);

  // dynamic set params during playing
  RED_ERR setProp(int property, const int64_t value);
  RED_ERR setProp(int property, const float value);
  int64_t getProp(int property, const int64_t default_value);
  float getProp(int property, const float default_value);

  RED_ERR getVideoCodecInfo(std::string &codec_info);
  RED_ERR getAudioCodecInfo(std::string &codec_info);
  RED_ERR getPlayUrl(std::string &url);
  int getPlayerState();

#if defined(__ANDROID__)
  RED_ERR setVideoSurface(JNIEnv *env, jobject jsurface);
#endif

#ifdef __HARMONY__
  RED_ERR setVideoSurface(OHNativeWindow *native_window);
  void getWidth(int32_t &width);
  void getHeight(int32_t &height);
#endif

#if defined(__APPLE__)
  UIView *initWithFrame(int type, CGRect cgrect);
#endif

  void *setInjectOpaque(void *opaque);
  void *setWeakThiz(void *weak_this);
  void *getWeakThiz();

  sp<Message> getMessage(bool block = false);
  RED_ERR recycleMessage(sp<Message> &msg);

private:
  CRedPlayer() = default;
  CRedPlayer(int id, MsgCallback msg_cb);
  RED_ERR init();
  void changeState(PlayerState target_state);
  bool checkStateStart();
  bool checkStatePause();
  bool checkStateSeek();
  void checkInputIsJson(const std::string &url);

private:
  const int mID{0};
  std::atomic<int64_t> mCurSeekPos{-1};
  std::atomic_bool mSeeking{false};
  bool mReleased{false};
  bool mRestart{false};
  bool mRestartFromBeginning{false};
  std::mutex mLock;
  std::mutex mSurfaceLock;
  std::mutex mThreadLock;
  std::thread mMsgThread;
  std::string mDataSource;
  std::atomic_bool playUrlReady{false};
  std::atomic<void *> mWeakThiz;
  sp<CRedCore> mRedCore;
  MessageQueue mMsgQueue;
  MsgCallback mMsgCb;
  PlayerState mPlayerState{MP_STATE_IDLE};
#if defined(__ANDROID__)
  jobject mSurface{nullptr};
#endif
#if defined(__HARMONY__)
  OHNativeWindow *mWindow{nullptr};
#endif
};

REDPLAYER_NS_END;
