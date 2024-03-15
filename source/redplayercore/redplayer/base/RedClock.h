#pragma once

#include "RedBase.h"
#include <mutex>

REDPLAYER_NS_BEGIN;

enum { CLOCK_AUDIO = 0, CLOCK_VIDEO = 1, CLOCK_EXTER = 2 };

class RedClock {
public:
  RedClock();
  void SetClock(double pts);
  void SetSpeed(double speed);
  void SetPause(bool bpause);
  double GetClock();
  void SetMasterClockType(int type);
  int GetMasterClockType();
  void SetClockAvaliable(bool bvalid);
  bool GetClockAvaliable();
  void SetClockSerial(int serial);
  int GetClockSerial();

private:
  std::mutex mLock;
  double mLastUpdateTime;
  double mPtsDrift;
  double mSpeed;
  bool mPause;
  int mMasterClockType; // 0 audio 1 video 2 extern
  int mSerial;
  bool mAvailable;

private:
  double GetCurrentTime();
};

REDPLAYER_NS_END;
