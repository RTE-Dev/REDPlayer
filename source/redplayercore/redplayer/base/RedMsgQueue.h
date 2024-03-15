#pragma once

#include "RedDef.h"
#include "RedError.h"
#include "RedMsg.h"

#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <stdint.h>

REDPLAYER_NS_BEGIN;

class Message {
public:
  Message() = default;
  Message(int what, int arg1 = 0, int arg2 = 0, void *obj1 = nullptr,
          void *obj2 = nullptr, int obj1_len = 0, int obj2_len = 0);
  ~Message();
  void clear();

public:
  int mWhat{RED_MSG_FLUSH};
  int mArg1{0};
  int mArg2{0};
  void *mObj1{nullptr};
  void *mObj2{nullptr};
  int64_t mTime{0};
};

class MessageQueue {
public:
  MessageQueue();
  ~MessageQueue();
  RED_ERR put(int what, int arg1 = 0, int arg2 = 0, void *obj1 = nullptr,
              void *obj2 = nullptr, int obj1_len = 0, int obj2_len = 0);
  RED_ERR remove(int what);
  RED_ERR recycle(sp<Message> &msg);
  RED_ERR start();
  RED_ERR flush();
  RED_ERR abort();
  sp<Message> get(bool block);

private:
  bool mAbort;
  std::mutex mMutex;
  std::condition_variable mCondition;
  std::list<sp<Message>> mQueue;
  std::list<sp<Message>> mRecycledQueue;
};

REDPLAYER_NS_END;
