#pragma once

#include "RedBase.h"
#include "RedDebug.h"
#include "RedError.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#ifdef __cplusplus
}
#endif
#include "reddecoder/audio/audio_decoder/audio_decoder.h"

#include "RedCore/module/sourcer/RedSourceController.h"
#include "base/RedBuffer.h"
#include "base/RedPacket.h"
#include "base/RedQueue.h"

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

class CAudioProcesser : public CRedThreadBase,
                        reddecoder::AudioDecodedCallback {
public:
  CAudioProcesser(int id, sp<CRedSourceController> &sourceManager,
                  const sp<VideoState> &state, NotifyCallback notify_cb);
  ~CAudioProcesser();
  void ThreadFunc();
  RED_ERR getFrame(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR Prepare(const sp<MetaData> &metadata);
  RED_ERR stop();
  void setConfig(const sp<CoreGeneralConfig> &config);
  void setNotifyCb(NotifyCallback notify_cb);
  void release();
  void resetEof();
  int getSerial();
  reddecoder::AudioCodecError
  on_decoded_frame(std::unique_ptr<reddecoder::Buffer> decoded_frame);
  void on_decode_error(reddecoder::AudioCodecError error);

private:
  CAudioProcesser() = default;
  RED_ERR Init();
  RED_ERR DecoderTransmit(AVPacket *pkt);
  RED_ERR ReadPacketOrBuffering(std::unique_ptr<RedAvPacket> &pkt);
  RED_ERR PerformStop();
  RED_ERR PerformFlush();
  RED_ERR ResetDecoderFormat();
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);
  bool checkAccurateSeek(const std::unique_ptr<CGlobalBuffer> &buffer);

private:
  std::mutex mLock;
  std::mutex mNotifyCbLock;
  std::mutex mThreadLock;
  std::condition_variable mCond;
  const int mID{0};
  bool mReleased{false};
  bool mEOF{false};
  sp<CoreGeneralConfig> mGeneralConfig;
  sp<CRedSourceController> mRedSourceController;
  sp<MetaData> mMetaData;
  sp<VideoState> mVideoState;
  std::unique_ptr<reddecoder::AudioDecoder> mAudioDecoder;
  std::unique_ptr<FrameQueue> mFrameQueue;
  NotifyCallback mNotifyCb;
};

REDPLAYER_NS_END;
