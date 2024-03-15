#pragma once

#include "RedBase.h"

REDPLAYER_NS_BEGIN;

#define DEFAULT_CAPACITY 10

class SpeedSampler {
public:
  SpeedSampler() = default;
  ~SpeedSampler() = default;
  void reset();
  float add();

private:
  int64_t mSamples[DEFAULT_CAPACITY];
  int mCapacity{DEFAULT_CAPACITY};
  int mCount{0};
  int mFirstIndex{0};
  int mNextIndex{0};
};

class SpeedSampler2 {
public:
  SpeedSampler2() = default;
  ~SpeedSampler2() = default;
  void reset(int sample_range);
  int64_t add(int quantity);
  int64_t getSpeed();
  int64_t getLastSpeed();

private:
  int64_t mSampleRange{0};
  int64_t mLastProfileTick{0};
  int64_t mLastProfileDuration{0};
  int64_t mLastProfileQuantity{0};
  int64_t mLastProfileSpeed{0};
};

REDPLAYER_NS_END;
