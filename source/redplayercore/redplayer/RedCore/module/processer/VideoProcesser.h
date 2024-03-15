#pragma once

#include "RedBase.h"
#include "RedDebug.h"
#include "RedError.h"

#include "AudioProcesser.h"
#include "base/RedBuffer.h"
#include "base/RedPacket.h"
#include "base/RedQueue.h"
#include "base/RedSampler.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#ifdef __cplusplus
}
#endif

#if defined(__ANDROID__)
#include "android/redplayer_android_def.h"
#include <android/native_window.h>
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

class CVideoProcesser : public CRedThreadBase,
                        reddecoder::VideoDecodedCallback {
public:
  CVideoProcesser(int id, sp<CRedSourceController> &sourceManager,
                  const sp<VideoState> &state, NotifyCallback notify_cb,
                  int serial = 0);
  ~CVideoProcesser();

  RED_ERR getFrame(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR Prepare(sp<MetaData> &metadata);
  RED_ERR stop();
  void setConfig(const sp<CoreGeneralConfig> &config);
  void setNotifyCb(NotifyCallback notify_cb);
  void release();
#if defined(__ANDROID__)
  RED_ERR setVideoSurface(const sp<RedNativeWindow> &surface);
#endif
  void ThreadFunc();
  int getSerial();
  void resetEof();
  sp<FrameQueue> frameQueue();

  reddecoder::VideoCodecError
  on_decoded_frame(std::unique_ptr<reddecoder::Buffer> decoded_frame);
  void on_decode_error(reddecoder::VideoCodecError error,
                       int internal_error_code = 0);

private:
  CVideoProcesser() = default;
  RED_ERR Init();
  RED_ERR DecoderTransmit(AVPacket *pkt);
  RED_ERR ReadPacketOrBuffering(std::unique_ptr<RedAvPacket> &pkt);
  RED_ERR PerformFlush();
  RED_ERR ResetDecoder();
  RED_ERR PerformStop();
  RED_ERR ResetDecoderFormat();
  void DecodeLastCacheGop();
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);
  bool checkAccurateSeek(const std::unique_ptr<CGlobalBuffer> &buffer);
  bool pktQueueFrontIsFlush();

private:
  std::mutex mLock;
  std::mutex mNotifyCbLock;
  std::mutex mSurfaceLock;
  std::mutex mThreadLock;
  std::condition_variable mCond;
  const int mID{0};
  int mWidth{0};
  int mHeight{0};
  int mFinishedSerial{-1};
  int mInputPacketCount{0};
  int mDecoderErrorCount{0};
  bool mDecoderRecovery{false};
  bool mFirstFrameDecoded{false};
  bool mFirstPacketInDecoder{false};
#if defined(__ANDROID__)
  sp<RedNativeWindow> mCurNativeWindow;
  sp<RedNativeWindow> mPrevNativeWindow;
#endif
  bool mCodecConfigured{false};
  bool mSurfaceUpdated{false};
  bool mEOF{false};
  bool mRefreshSession{false};
  bool mIsHevc{false};
  bool mIdrBasedIdentified{true};
  bool mReleased{false};
  sp<CoreGeneralConfig> mGeneralConfig;
  sp<CRedSourceController> mRedSourceController;
  sp<FrameQueue> mFrameQueue;
  std::unique_ptr<RedAvPacket> mPendingPkt;
  std::unique_ptr<reddecoder::Buffer> mBuffer;
  std::unique_ptr<reddecoder::VideoDecoder> mVideoDecoder;
  std::list<std::shared_ptr<RedAvPacket>> mPktQueue;
  sp<MetaData> mMetaData;
  sp<VideoState> mVideoState;
  NotifyCallback mNotifyCb;
  SpeedSampler mSpeedSampler;
  std::condition_variable mSurfaceSet;
};

REDPLAYER_NS_END;
