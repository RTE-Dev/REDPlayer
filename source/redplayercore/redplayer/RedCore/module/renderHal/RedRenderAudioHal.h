#pragma once

#include "RedBase.h"
#include "RedDebug.h"
#include "RedError.h"

#include "RedCore/module/processer/AudioProcesser.h"
#include "SoundTouchHal.h"
#include "base/RedBuffer.h"
#include "base/RedClock.h"
#include "base/RedQueue.h"
#include "redrender/audio/audio_render_factory.h"
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
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

typedef void (*AudioRenderAudioCallback)(void *userdata, char *stream,
                                         int &len);
class AudioCallbackClass : public redrender::audio::AudioCallback {
public:
  AudioCallbackClass(void *userData, AudioRenderAudioCallback callback)
      : AudioCallback(userData) {
    _callback = callback;
  }
  ~AudioCallbackClass() = default;

  void GetBuffer(uint8_t *stream, int len) override {
    if (_callback) {
      _callback(user_data_, reinterpret_cast<char *>(stream), len);
    }
  };

private:
  AudioRenderAudioCallback _callback;
};

class CRedRenderAudioHal : public CRedThreadBase {
public:
  CRedRenderAudioHal(int id, sp<CAudioProcesser> &processer,
                     const sp<VideoState> &state, NotifyCallback notify_cb);
  ~CRedRenderAudioHal();
  virtual void ThreadFunc();
  RED_ERR Prepare(sp<MetaData> &metadata);
  RED_ERR pause();
  RED_ERR start();
  RED_ERR stop();
  RED_ERR flush();
  void setConfig(const sp<CoreGeneralConfig> &config);
  void setNotifyCb(NotifyCallback notify_cb);
  void release();
  float getPlaybackRate();
  void setPlaybackRate(const float rate);
  float getVolume();
  void setVolume(const float left_volume, const float right_volume);
  void setMute(bool mute);

private:
  CRedRenderAudioHal() = default;
  RED_ERR PerformStart();
  RED_ERR PerformPause();
  RED_ERR PerformStop();
  RED_ERR PerformFlush();
  RED_ERR ReadFrame(std::unique_ptr<CGlobalBuffer> &buffer);
  RED_ERR Init();
  static void AudioDataCb(void *userdata, char *data, int &len);
  void GetAudioData(char *data, int &len);
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);
  redrender::audio::AudioFormat
  FfmegAudioFmtToRedRenderAudioFmt(enum AVSampleFormat sampleFmt);
  AVSampleFormat
  RedRenderAudioFmtToFfmegAudioFmt(redrender::audio::AudioFormat fmt);
  int ResampleAudioData(std::unique_ptr<CGlobalBuffer> &buffer);

private:
  const int mID{0};
  sp<CAudioProcesser> mAudioProcesser;
  std::mutex mLock;
  std::mutex mNotifyCbLock;
  std::condition_variable mCond;
  std::unique_ptr<CGlobalBuffer> mAudioBuffer;
  uint8_t *mAudioBuf{nullptr};
  uint8_t *mAudioBuf1{nullptr};
  int mAudioBufSize{0};
  unsigned int mAudioBuf1Size{0};
  unsigned int mAudioNewBufSize{0};
  short *mAudioNewBuf{nullptr};
  bool mAbort{false};
  bool mPaused{false};
  bool mPlaybackRateChanged{false};
  bool mVolumeChanged{false};
  bool mFirstFrameDecoded{false};
  sp<MetaData> mMetaData;
  sp<CoreGeneralConfig> mGeneralConfig;
  sp<VideoState> mVideoState;
  double mAudioDelay{0.0};
  float mPlaybackRate{1.0};
  float mLeftVolume{1.0};
  float mRightVolume{1.0};
  int mLastReadPos{0};
  int mBytesPerSec{0};
  std::atomic_bool mMute{false};
  NotifyCallback mNotifyCb;
  struct SwrContext *mSwrContext{nullptr};
  soundtouch::SoundTouch *mSoundTouchHandle{nullptr};
  // redrender audio
  std::unique_ptr<redrender::audio::IAudioRender> mAudioRender{nullptr};
  redrender::audio::AudioInfo mDesired;
  redrender::audio::AudioInfo mObtained;
  redrender::audio::AudioInfo mCurrentInfo;
  std::unique_ptr<redrender::audio::AudioCallback> mAudioCallbackClass{nullptr};
};

REDPLAYER_NS_END;
