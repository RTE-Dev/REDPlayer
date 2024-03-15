/*
 * RedQueue.cpp
 *
 *  Created on: 2022年7月25日
 *      Author: liuhongda
 */
#include "RedQueue.h"
#include "RedBase.h"
#include "RedDebug.h"
#include "RedError.h"
#include <algorithm>

#define TAG "RedQueue"

REDPLAYER_NS_BEGIN;

CRedThreadBase::CRedThreadBase() { mAbort = false; }
CRedThreadBase::~CRedThreadBase() {
  if (mThread.joinable()) {
    mThread.join();
  }
}
void CRedThreadBase::run() {
  if (mAbort)
    return;
  try {
    mThread = std::thread(std::bind(&CRedThreadBase::ThreadFunc, this));
  } catch (const std::system_error &e) {
    AV_LOGE(TAG, "[%s:%d] Exception caught: %s!\n", __FUNCTION__, __LINE__,
            e.what());
    return;
  } catch (...) {
    AV_LOGE(TAG, "[%s:%d] Exception caught!\n", __FUNCTION__, __LINE__);
    return;
  }
}

PktQueue::PktQueue(int type) /*: mType(type)*/ {}

RED_ERR PktQueue::putPkt(std::unique_ptr<RedAvPacket> &pkt) {
  std::unique_lock<std::mutex> lck(mLock);
  AVPacket *packet = pkt ? pkt->GetAVPacket() : nullptr;
  mBytes += packet ? packet->size : 0;
  mDuration +=
      packet ? std::max(packet->duration, (int64_t)MIN_PKT_DURATION) : 0;
  mPktQueue.push(std::move(pkt));
  mNotEmptyCond.notify_one();
  return OK;
}

RED_ERR PktQueue::getPkt(std::unique_ptr<RedAvPacket> &pkt, bool block) {
  std::unique_lock<std::mutex> lck(mLock);
  if (mPktQueue.empty() && !block)
    return ME_RETRY;
  while (mPktQueue.empty() && block) {
    if (mAbort) {
      return OK;
    }
    if (mNotEmptyCond.wait_for(lck, std::chrono::seconds(1)) ==
        std::cv_status::timeout) {
      AV_LOGV(TAG, "pktqueue[%d] EMPTY for 1s!\n", mType);
    }
  }
  pkt = std::move(mPktQueue.front());
  AVPacket *packet = pkt ? pkt->GetAVPacket() : nullptr;
  mBytes -= packet ? packet->size : 0;
  mDuration -=
      packet ? std::max(packet->duration, (int64_t)MIN_PKT_DURATION) : 0;
  mPktQueue.pop();
  return OK;
}

bool PktQueue::frontIsFlush() {
  std::unique_lock<std::mutex> lck(mLock);
  if (mPktQueue.empty()) {
    return false;
  }
  if (!mPktQueue.front()) {
    return false;
  }
  return mPktQueue.front()->IsFlushPacket();
}

void PktQueue::flush() {
  std::unique_lock<std::mutex> lck(mLock);
  int flush_pkt_count = 0;
  while (!mPktQueue.empty()) {
    auto pkt = std::move(mPktQueue.front());
    if (pkt->IsFlushPacket()) {
      flush_pkt_count++;
    }
    mPktQueue.pop();
  }
  for (int i = 0; i < flush_pkt_count; ++i) {
    std::unique_ptr<RedAvPacket> flush_pkt(new RedAvPacket(PKT_TYPE_FLUSH));
    mPktQueue.push(std::move(flush_pkt));
  }
  mBytes = 0;
  mDuration = 0;
}

void PktQueue::abort() {
  std::unique_lock<std::mutex> lck(mLock);
  mAbort = true;
  mNotEmptyCond.notify_one();
}

void PktQueue::clear() {
  std::unique_lock<std::mutex> lck(mLock);
  while (!mPktQueue.empty()) {
    mPktQueue.pop();
  }
  mBytes = 0;
  mDuration = 0;
}

size_t PktQueue::size() {
  std::unique_lock<std::mutex> lck(mLock);
  return mPktQueue.size();
}

int64_t PktQueue::bytes() {
  std::unique_lock<std::mutex> lck(mLock);
  return mBytes;
}

int64_t PktQueue::duration() {
  std::unique_lock<std::mutex> lck(mLock);
  return mDuration;
}

FrameQueue::FrameQueue(size_t capacity, int type)
    : mCapacity(capacity) /*, mType(type)*/ {}

RED_ERR FrameQueue::putFrame(std::unique_ptr<CGlobalBuffer> &frame) {
  std::unique_lock<std::mutex> lck(mLock);
  while (mFrameQueue.size() >= mCapacity) {
    if (mAbort) {
      return OK;
    }
    if (mNotFullCond.wait_for(lck, std::chrono::seconds(3)) ==
        std::cv_status::timeout) {
      AV_LOGV(TAG, "framequeue[%d] FULL for 3s!\n", mType);
    }
  }
  mFrameQueue.push(std::move(frame));
  mNotEmptyCond.notify_one();
  return OK;
}

RED_ERR FrameQueue::getFrame(std::unique_ptr<CGlobalBuffer> &frame) {
  std::unique_lock<std::mutex> lck(mLock);
  while (mFrameQueue.empty()) {
    if (mAbort) {
      return OK;
    }
    if (mWakeup) {
      mWakeup = false;
      return OK;
    }
    if (mNotEmptyCond.wait_for(lck, std::chrono::seconds(1)) ==
        std::cv_status::timeout) {
      AV_LOGV(TAG, "framequeue[%d] EMPTY for 1s!\n", mType);
    }
  }
  frame = std::move(mFrameQueue.front());
  mFrameQueue.pop();
  mNotFullCond.notify_one();
  return OK;
}

void FrameQueue::flush() {
  std::unique_lock<std::mutex> lck(mLock);
  while (!mFrameQueue.empty()) {
    mFrameQueue.pop();
  }
  mNotFullCond.notify_one();
}

void FrameQueue::abort() {
  std::unique_lock<std::mutex> lck(mLock);
  mAbort = true;
  mNotFullCond.notify_one();
  mNotEmptyCond.notify_one();
}

void FrameQueue::wakeup() {
  std::unique_lock<std::mutex> lck(mLock);
  mWakeup = true;
  mNotEmptyCond.notify_one();
}

size_t FrameQueue::size() {
  std::unique_lock<std::mutex> lck(mLock);
  return mFrameQueue.size();
}

REDPLAYER_NS_END;
