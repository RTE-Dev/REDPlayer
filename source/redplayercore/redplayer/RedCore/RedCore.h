#pragma once

#include "RedBase.h"
#include "RedCore/module/processer/AudioProcesser.h"
#include "RedCore/module/processer/VideoProcesser.h"
#include "RedCore/module/renderHal/RedRenderAudioHal.h"
#include "RedCore/module/renderHal/RedRenderVideoHal.h"
#include "RedCore/module/sourcer/RedSourceController.h"
#include "RedCore/module/sourcer/format/redioapplication.h"
#include "RedError.h"
#include "RedMeta.h"
#include "RedProp.h"
#include "base/RedConfig.h"
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>

extern "C" {
#include "libavutil/dict.h"
#include "libavutil/pixdesc.h"
}

REDPLAYER_NS_BEGIN;

void globalSetInjectCallback(InjectCallback cb);

class CRedPlayer;
class CRedCore {
public:
  CRedCore() = default;
  CRedCore(int id, NotifyCallback notify_cb);
  ~CRedCore();

  int id() const;
  RED_ERR init();

  void setDataSource(std::string url);
  void setDataSourceFd(int64_t fd);
  RED_ERR prepareAsync();
  RED_ERR start();
  RED_ERR startFrom(int64_t msec);
  RED_ERR pause();
  RED_ERR seekTo(int64_t msec, bool flush_queue = true);
  RED_ERR stop();
  void release();
  status_t getCurrentPosition(int64_t &pos_ms);
  status_t getDuration(int64_t &dur_ms);
  void setVolume(const float left_volume, const float right_volume);
  void setMute(bool mute);
  void setLoop(int loop_count);
  int getLoop();

  RED_ERR setConfig(int type, std::string name, std::string value);
  RED_ERR setConfig(int type, std::string name, int64_t value);
  // dynamic set params during playing
  RED_ERR setProp(int property, const int64_t value);
  RED_ERR setProp(int property, const float value);
  int64_t getProp(int property, const int64_t default_value);
  float getProp(int property, const float default_value);
  void setNotifyCb(NotifyCallback cb);
  void *setInjectOpaque(void *opaque);
  void *getInjectOpaque();
  sp<RedDict> getConfig(int config_type);
  RED_ERR getVideoCodecInfo(std::string &codec_info);
  RED_ERR getAudioCodecInfo(std::string &codec_info);
#if defined(__ANDROID__) || defined(__HARMONY__)
  RED_ERR setVideoSurface(const sp<RedNativeWindow> &surface);
  void getWidth(int32_t &width);
  void getHeight(int32_t &height);
#endif
#if defined(__APPLE__)
  UIView *initWithFrame(int type, CGRect cgrect);
#endif
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);

private:
  RED_ERR PerformConfigs();
  RED_ERR resetConfigs();
  RED_ERR PerformPlayerConfig();
  RED_ERR PerformFormatConfig();
  RED_ERR PerformCodecConfig();
  RED_ERR PerformSwsConfig();
  RED_ERR PerformStart();
  RED_ERR PerformPrepare();
  RED_ERR PerformSeek(int64_t msec);
  RED_ERR PerformStop();
  RED_ERR PerformPause();
  RED_ERR PerformFlush();
  void checkHighFps();
  void PrepareStream(sp<MetaData> &metadata);
  void PreparedCb(sp<MetaData> &metadata);
  void parseVideoExtraData(TrackInfo &info);
  void dumpFormatInfo();
  void setPlaybackRate(float rate);

public:
  sp<VideoState> mVideoState;

private:
  std::mutex mLock;
  std::mutex mAudioStreamLock;
  std::mutex mVideoStreamLock;
  std::mutex mNotifyCbLock;
  std::mutex mSurfaceLock;
  std::string mUrl;
  std::atomic<void *> mInjectOpaque;

  const int mID{0};
  bool mPausedByClient{false};
  std::atomic_bool mSeeking{false};
  std::atomic_bool mCompleted{false};
  bool mAbort{false};
  int64_t mFd{-1};

  sp<CRedSourceController> mRedSourceController;
  sp<CVideoProcesser> mVideoProcesser;
  sp<CAudioProcesser> mAudioProcesser;
  sp<CRedRenderVideoHal> mRedRenderVideoHal;
  sp<CRedRenderAudioHal> mRedRenderAudioHal;
  sp<CoreGeneralConfig> mGeneralConfig;
  sp<MetaData> mMetaData;
  sp<RedDict> mFormatConfig;
  sp<RedDict> mCodecConfig;
  sp<RedDict> mSwsConfig;
  sp<RedDict> mSwrConfig;
  sp<RedDict> mPlayerConfig;
#if defined(__ANDROID__) || defined(__HARMONY__)
  sp<RedNativeWindow> mNativeWindow;
#endif

  RedApplicationContext *mAppCtx{nullptr};

  std::atomic<int64_t> mCurSeekPos{-1};
  NotifyCallback mNotifyCb;
};

REDPLAYER_NS_END;
