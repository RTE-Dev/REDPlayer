/*
 * RedMsgQueue.cpp
 *
 *  Created on: 2022年10月19日
 *      Author: liuhongda
 */

#include "RedMsgQueue.h"
#include "RedLog.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/mem.h"
#ifdef __cplusplus
}
#endif

#define TAG "Message"

REDPLAYER_NS_BEGIN;

Message::Message(int what, int arg1, int arg2, void *obj1, void *obj2,
                 int obj1_len, int obj2_len)
    : mWhat(what), mArg1(arg1), mArg2(arg2) {
  mTime = CurrentTimeMs();
  if (obj1 && obj1_len > 0) {
    mObj1 = av_mallocz(obj1_len * sizeof(uint8_t));
    if (mObj1) {
      memcpy(mObj1, obj1, obj1_len);
    }
  }
  if (obj2 && obj2_len > 0) {
    mObj2 = av_mallocz(obj2_len * sizeof(uint8_t));
    if (mObj2) {
      memcpy(mObj2, obj2, obj2_len);
    }
  }
}

Message::~Message() { clear(); }

void Message::clear() {
  mWhat = RED_MSG_FLUSH;
  mArg1 = 0;
  mArg2 = 0;
  mTime = 0;
  if (mObj1) {
    av_freep(&mObj1);
  }
  if (mObj2) {
    av_freep(&mObj2);
  }
}

MessageQueue::MessageQueue() {
  mAbort = false;
  mQueue.clear();
  mRecycledQueue.clear();
}

MessageQueue::~MessageQueue() {
  mAbort = false;
  mQueue.clear();
  mRecycledQueue.clear();
}

RED_ERR MessageQueue::put(int what, int arg1, int arg2, void *obj1, void *obj2,
                          int obj1_len, int obj2_len) {
  Autolock lck(mMutex);
  sp<Message> msg;
  bool notify = false;
  if (mRecycledQueue.empty()) {
    try {
      msg = std::make_shared<Message>(what, arg1, arg2, obj1, obj2, obj1_len,
                                      obj2_len);
    } catch (const std::bad_alloc &e) {
      AV_LOGE(TAG, "[%s:%d] Exception caught: %s!\n", __FUNCTION__, __LINE__,
              e.what());
      return NO_MEMORY;
    } catch (...) {
      AV_LOGE(TAG, "[%s:%d] Exception caught!\n", __FUNCTION__, __LINE__);
      return NO_MEMORY;
    }
  } else {
    msg = mRecycledQueue.front();
    mRecycledQueue.pop_front();
    if (!msg) {
      return NO_MEMORY;
    }
    msg->mWhat = what;
    msg->mArg1 = arg1;
    msg->mArg2 = arg2;
    msg->mTime = CurrentTimeMs();
    if (obj1 && obj1_len > 0) {
      msg->mObj1 = av_mallocz(obj1_len * sizeof(uint8_t));
      if (msg->mObj1) {
        memcpy(msg->mObj1, obj1, obj1_len);
      }
    }
    if (obj2 && obj2_len > 0) {
      msg->mObj2 = av_mallocz(obj2_len * sizeof(uint8_t));
      if (msg->mObj2) {
        memcpy(msg->mObj2, obj2, obj2_len);
      }
    }
  }

  notify = mQueue.empty();
  if (!msg) {
    AV_LOGD(TAG, "[%s] return null msg due to unknow reason %d %d %d %d\n",
            __func__, mAbort, what, arg1, arg2);
  }
  mQueue.emplace_back(msg);

  if (notify) {
    mCondition.notify_one();
  }

  return OK;
}

RED_ERR MessageQueue::remove(int what) {
  Autolock lck(mMutex);
  for (auto it = mQueue.begin(); it != mQueue.end();) {
    if ((*it) && (*it)->mWhat == what) {
      (*it)->clear();
      mRecycledQueue.emplace_back((*it));
      it = mQueue.erase(it);
    } else {
      ++it;
    }
  }
  return OK;
}

RED_ERR MessageQueue::recycle(sp<Message> &msg) {
  Autolock lck(mMutex);
  if (!msg) {
    return OK;
  }
  msg->clear();
  mRecycledQueue.emplace_back(msg);
  return OK;
}

RED_ERR MessageQueue::start() {
  Autolock lck(mMutex);
  mAbort = false;
  return OK;
}

RED_ERR MessageQueue::flush() {
  Autolock lck(mMutex);
  for (auto it = mQueue.begin(); it != mQueue.end();) {
    (*it)->clear();
    mRecycledQueue.emplace_back((*it));
    it = mQueue.erase(it);
  }
  return OK;
}

RED_ERR MessageQueue::abort() {
  Autolock lck(mMutex);
  mAbort = true;
  mCondition.notify_one();
  return OK;
}

sp<Message> MessageQueue::get(bool block) {
  std::unique_lock<std::mutex> lck(mMutex);
  sp<Message> ret;
  while (mQueue.empty()) {
    if (!block) {
      return ret;
    } else if (mAbort) {
      return ret;
    } else {
      mCondition.wait(lck);
    }
  }
  if (mAbort) {
    return ret;
  }
  ret = mQueue.front();
  mQueue.pop_front();
  if (!ret) {
    AV_LOGD(TAG, "[%s] return null msg due to unknow reason %d\n", __func__,
            mAbort);
  }
  return ret;
}

REDPLAYER_NS_END;
