/*
 * RedBase.h
 *
 *  Created on: 2022年3月24日
 *      Author: liuhongda
 */

#pragma once

#include <stdint.h>

#include <mutex>

#define REDPLAYER_NS_BEGIN namespace redPlayer_ns {
#define REDPLAYER_NS_END }

#define DISALLOW_EVIL_CONSTRUCTORS(name)                                       \
  name(const name &);                                                          \
  name &operator=(const name &)

#define sp std::shared_ptr
#define wp std::weak_ptr
#define Autolock std::lock_guard<std::mutex>

static inline int64_t CurrentTimeUs() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

static inline int64_t CurrentTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
