#include "RedClock.h"
#include <chrono>
#include <cmath>
#include <iostream>

REDPLAYER_NS_BEGIN;

RedClock::RedClock()
    : mSpeed(1.0f), mPause(true), mMasterClockType(CLOCK_EXTER), mSerial(0),
      mAvailable(false) {
  mLastUpdateTime = GetCurrentTime();
  mPtsDrift = 0 - mLastUpdateTime;
}

void RedClock::SetClock(double pts) {
  std::lock_guard<std::mutex> lck(mLock);
  double ctime = GetCurrentTime();
  mPtsDrift = pts - ctime;
  mLastUpdateTime = ctime;
}

double RedClock::GetClock() {
  std::lock_guard<std::mutex> lck(mLock);
  if (!mAvailable) {
    return NAN;
  }
  double ctime = GetCurrentTime();
  if (mPause)
    ctime = mLastUpdateTime;
  return mPtsDrift + ctime - (ctime - mLastUpdateTime) * (1.0f - mSpeed);
}

void RedClock::SetSpeed(double speed) {
  std::lock_guard<std::mutex> lck(mLock);
  mSpeed = speed;
}

void RedClock::SetPause(bool bpause) {
  std::lock_guard<std::mutex> lck(mLock);
  mPause = bpause;
}

void RedClock::SetMasterClockType(int type) {
  std::lock_guard<std::mutex> lck(mLock);
  mMasterClockType = type;
}

int RedClock::GetMasterClockType() {
  std::lock_guard<std::mutex> lck(mLock);
  return mMasterClockType;
}

void RedClock::SetClockAvaliable(bool bvalid) {
  std::lock_guard<std::mutex> lck(mLock);
  mAvailable = bvalid;
}

bool RedClock::GetClockAvaliable() {
  std::lock_guard<std::mutex> lck(mLock);
  return mAvailable;
}

void RedClock::SetClockSerial(int serial) {
  std::lock_guard<std::mutex> lck(mLock);
  mSerial = serial;
}

int RedClock::GetClockSerial() {
  std::lock_guard<std::mutex> lck(mLock);
  return mSerial;
}

double RedClock::GetCurrentTime() {
  return static_cast<double>(CurrentTimeUs() / 1000000.0);
}

REDPLAYER_NS_END;
