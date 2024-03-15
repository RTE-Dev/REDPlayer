#pragma once

#include "RedBase.h"
#include "RedDebug.h"
#include "RedDef.h"
#include "RedError.h"
#include "RedLog.h"
#include "RedSource.h"
#include "RedSourceCommon.h"
#include "base/RedConfig.h"
#include "base/RedPacket.h"
#include "base/RedQueue.h"

#include <iostream>
#include <stdint.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

REDPLAYER_NS_BEGIN;

#define DEFAULT_RECONNECT_AVFORMAR_OPEN_INPUT 3
#define MAX_SEEK_TIME 20
#define DYNAMIC_BUFFER_LENGTH 7

using PrepareCallBack = std::function<void(sp<MetaData> &)>;

using CRedSource = redsource::RedSource;

class CRedSourceController : public CRedThreadBase {
public:
  CRedSourceController(int id, const sp<VideoState> &state,
                       NotifyCallback notify_cb);
  ~CRedSourceController();

  // base
  void ThreadFunc();

public:
  RED_ERR open(std::string url);
  RED_ERR seek(int64_t msec);
  RED_ERR stop();
  RED_ERR prepareAsync();
  RED_ERR getPacket(std::unique_ptr<RedAvPacket> &pkt, int stream_type,
                    bool block);
  RED_ERR putPacket(std::unique_ptr<RedAvPacket> &pkt, int stream_type);
  bool pktQueueFrontIsFlush(int stream_type);
  void pktQueueAbort(int stream_type);
  bool isBuffering();
  void setNotifyCb(NotifyCallback notify_cb);
  void setPrepareCb(PrepareCallBack cb);
  void setConfig(const sp<CoreGeneralConfig> &config);
  void release();
  void toggleBuffering(bool buffering);
  int getSerial();

private:
  CRedSourceController() = default;
  RED_ERR PerformStop();
  RED_ERR PerformFlush();
  RED_ERR SetMetaData();
  RED_ERR putFlushPacket();
  RED_ERR putEofPacket();
  sp<PktQueue> pktQueue(int stream_type);
  bool isBufferFull();
  bool checkDropNonRefFrame(AVPacket *pkt);
  int getErrorType(int errorCode);
  void updateCacheStatistic();
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);
  void checkBuffering();

private:
  PrepareCallBack mPrepareCb;
  std::mutex mLock;
  std::mutex mNotifyCbLock;
  std::mutex mPreparedCbLock;
  std::mutex mThreadLock;
  std::condition_variable mCond;
  std::string mUrl;
  const int mID{0};
  int mAudioIndex{-1};
  int mVideoIndex{-1};
  bool mEOF{false};
  bool mBuffering{false};
  bool mSeekBuffering{false};
  bool mFirstVideoPktInPktQueue{false};
  bool mReleased{false};
  std::unordered_map<int, sp<PktQueue>> mPktQueueMap;
  sp<CoreGeneralConfig> mGeneralConfig;
  sp<CRedSource> mRedSource;
  sp<MetaData> mMetaData;
  sp<VideoState> mVideoState;
  NotifyCallback mNotifyCb;
  int mMaxBufferSize{MAX_QUEUE_SIZE};
  int mBufferingPercent{0};
};
REDPLAYER_NS_END;
