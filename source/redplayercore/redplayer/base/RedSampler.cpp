#include "RedSampler.h"

REDPLAYER_NS_BEGIN;

void SpeedSampler::reset() {
  for (int i = 0; i < mCapacity; i++) {
    mSamples[i] = 0;
  }
  mCount = 0;
  mFirstIndex = 0;
  mNextIndex = 0;
}

float SpeedSampler::add() {
  int64_t now = CurrentTimeMs();
  mSamples[mNextIndex] = now;
  mNextIndex++;
  mNextIndex %= mCapacity;
  if (mCount + 1 >= mCapacity) {
    mFirstIndex++;
    mFirstIndex %= mCapacity;
  } else {
    mCount++;
  }

  if (mCapacity < 2 || now - mSamples[mFirstIndex] <= 0) {
    return 0.0f;
  }

  float samples_per_second =
      1000.0f * (mCount - 1) / (now - mSamples[mFirstIndex]);

  return samples_per_second;
}

void SpeedSampler2::reset(int sample_range) {
  mSampleRange = sample_range;
  mLastProfileTick = CurrentTimeMs();
  mLastProfileDuration = 0;
  mLastProfileQuantity = 0;
  mLastProfileSpeed = 0;
}

int64_t SpeedSampler2::add(int quantity) {
  if (quantity < 0)
    return 0;

  int64_t sample_range = mSampleRange;
  int64_t last_tick = mLastProfileTick;
  int64_t last_duration = mLastProfileDuration;
  int64_t last_quantity = mLastProfileQuantity;
  int64_t now = CurrentTimeMs();
  int64_t elapsed = std::abs(now - last_tick);
  if ((elapsed < 0 || elapsed >= sample_range) && sample_range > 0) {
    // overflow, reset to initialized state
    mLastProfileTick = now;
    mLastProfileDuration = sample_range;
    mLastProfileQuantity = quantity;
    mLastProfileSpeed = quantity * 1000 / sample_range;
    return mLastProfileSpeed;
  }

  int64_t new_quantity = last_quantity + quantity;
  int64_t new_duration = last_duration + elapsed;
  if (new_duration > sample_range && new_duration > 0) {
    new_quantity = new_quantity * sample_range / new_duration;
    new_duration = sample_range;
  }

  mLastProfileTick = now;
  mLastProfileDuration = new_duration;
  mLastProfileQuantity = new_quantity;
  if (new_duration > 0)
    mLastProfileSpeed = new_quantity * 1000 / new_duration;

  return mLastProfileSpeed;
}

int64_t SpeedSampler2::getSpeed() {
  int64_t sample_range = mSampleRange;
  int64_t last_tick = mLastProfileTick;
  int64_t last_quantity = mLastProfileQuantity;
  int64_t last_duration = mLastProfileDuration;
  int64_t now = CurrentTimeMs();
  int64_t elapsed = std::abs(now - last_tick);
  if (elapsed < 0 || elapsed >= sample_range)
    return 0;

  int64_t new_quantity = last_quantity;
  int64_t new_duration = last_duration + elapsed;
  if (new_duration > sample_range && new_duration > 0) {
    new_quantity = new_quantity * sample_range / new_duration;
    new_duration = sample_range;
  }

  if (new_duration <= 0)
    return 0;

  return new_quantity * 1000 / new_duration;
}

int64_t SpeedSampler2::getLastSpeed() { return mLastProfileSpeed; }

REDPLAYER_NS_END;
