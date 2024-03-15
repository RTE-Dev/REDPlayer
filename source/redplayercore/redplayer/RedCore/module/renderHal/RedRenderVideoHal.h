#pragma once

#include "RedBase.h"
#include "RedDebug.h"
#include "RedError.h"

#include "RedCore/module/processer/VideoProcesser.h"
#include "base/RedBuffer.h"
#include "base/RedClock.h"
#include "base/RedQueue.h"
#include "base/RedSampler.h"
#include "redrender/video/video_renderer_factory.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif

#include <iostream>
#include <stdint.h>
#include <unistd.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <atomic>
#include <chrono>

REDPLAYER_NS_BEGIN;

#if defined(__APPLE__)
enum { RENDER_TYPE_OPENGL = 0, RENDER_TYPE_METAL = 1, RENDER_TYPE_AVSBDL = 2 };
#endif

class CRedRenderVideoHal : public CRedThreadBase {
public:
  CRedRenderVideoHal(int id, sp<CVideoProcesser> &processer,
                     const sp<VideoState> &state, NotifyCallback notify_cb);
  ~CRedRenderVideoHal();
  RED_ERR Prepare(sp<MetaData> &metadata);
#if defined(__ANDROID__)
  RED_ERR setVideoSurface(const sp<RedNativeWindow> &surface);
#endif
#if defined(__APPLE__)
  UIView *initWithFrame(int type, CGRect cgrect);
#endif
  // base
  void ThreadFunc();
  RED_ERR pause();
  RED_ERR start();
  RED_ERR stop();
  RED_ERR flush();
  void setConfig(const sp<CoreGeneralConfig> &config);
  void setNotifyCb(NotifyCallback notify_cb);
  void release();

private:
  CRedRenderVideoHal() = default;
  double ComputeDelay(double delay);
  double ComputeDuration(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR ReadFrame(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR PerformStart();
  RED_ERR PerformPause();
  RED_ERR PerformStop();
  RED_ERR PerformFlush();
  RED_ERR Init();
  RED_ERR RenderFrame(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR ConvertPixelFormat(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR UpdateVideoFrameMetaData();
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);

private:
  struct FrameTick {
    int serial{0};
    double time{0.0};
    double pts{0.0};
    double duration{0.0};
  };
  std::mutex mLock;
  std::mutex mPauseLock;
  std::mutex mNotifyCbLock;
  std::mutex mSurfaceLock;
  std::mutex mThreadLock;
  std::condition_variable mCond;
  std::condition_variable mPausedCond;
  std::condition_variable mSurfaceSet;
  const int mID{0};
  bool mForceRefresh{false};
  bool mPaused{false};
  bool mSurfaceUpdate{false};
  bool mReleased{false};
  bool mMetaDataSetuped{false};
  bool mRenderSetuped{false};
  sp<CoreGeneralConfig> mGeneralConfig;
  sp<CVideoProcesser> mVideoProcesser;
  sp<MetaData> mMetaData;
  sp<VideoState> mVideoState;
  NotifyCallback mNotifyCb;
  SpeedSampler mSpeedSampler;
  FrameTick mFrameTick;

  // redrender video
  std::unique_ptr<RedRender::VideoRenderer> mVideoRender{nullptr};
  RedRender::VideoFrameMetaData mVideoFrameMetaData;
  RedRender::VideoMediaCodecBufferContext mVideoMediaCodecBufferContext;
#if defined(__ANDROID__)
  sp<RedNativeWindow> mNativeWindow;
#endif
  RedRender::VRClusterType mClusterType{RedRender::VRClusterTypeUnknown};
  SwsContext *mSwsContext{nullptr};
};

REDPLAYER_NS_END;
