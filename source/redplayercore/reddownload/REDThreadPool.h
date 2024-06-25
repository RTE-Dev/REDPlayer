#pragma once

#include <stdio.h>

#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "REDDownloadTask.h"
using namespace std;

class REDThreadPool {
public:
  static REDThreadPool *getinstance();
  ~REDThreadPool();

  void initialize_threadpool(int pool_size);
  void destroy_threadpool();
  void add_task(shared_ptr<REDDownLoadTask> task, bool bpreload);
  void delete_task(shared_ptr<REDDownLoadTask> task);
  void move_task_to_head(shared_ptr<REDDownLoadTask> task);
  void clear(bool bpreload);

private:
  REDThreadPool(int pool_size = 1);
  REDThreadPool() = default;
  void stoptask();
  const REDThreadPool &operator=(const REDThreadPool &instance);
  static REDThreadPool mthreadpool;
  std::shared_ptr<REDDownLoadTask> take_task();
  void thread_loop(std::string thread_name);
  int m_pool_size;
  volatile bool m_pool_start;
  std::mutex m_mutex;
  std::condition_variable m_cond;

private:
  typedef std::shared_ptr<REDDownLoadTask> taskptr; // TODO: use weak_ptr
  typedef std::vector<std::shared_ptr<std::thread>> m_threads;
  m_threads mthreads_;
  list<taskptr> mprequeue;
  vector<taskptr> mplayqueue;
};
