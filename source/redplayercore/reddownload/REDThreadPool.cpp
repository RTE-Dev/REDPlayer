#include "REDThreadPool.h"

#include "REDURLParser.h"
#include "RedDownloadConfig.h"
#include "RedLog.h"

#define LOG_TAG "RedThreadPool"

REDThreadPool REDThreadPool::mthreadpool(1);
REDThreadPool *REDThreadPool::getinstance() { return &mthreadpool; }
REDThreadPool::REDThreadPool(int pool_size)
    : m_pool_size(pool_size), m_pool_start(false), m_mutex(), m_cond() {
  // initialize_threadpool();
}

REDThreadPool::~REDThreadPool() {
  if (m_pool_start) {
    destroy_threadpool();
  }
}

void REDThreadPool::initialize_threadpool(int pool_size) {
  m_pool_start = true;
  m_pool_size = pool_size;
  mthreads_.reserve(m_pool_size);
  for (int i = 0; i < m_pool_size; ++i) {
    std::string thread_name = "REDThreadPool_";
    thread_name += std::to_string(i);
    mthreads_.push_back(std::make_shared<std::thread>(
        std::bind(&REDThreadPool::thread_loop, this, thread_name)));
  }
}

void REDThreadPool::destroy_threadpool() {
  {
    std::unique_lock<mutex> lock(m_mutex);
    m_pool_start = false;
    m_cond.notify_all();
  }
  for (auto it = mthreads_.begin(); it != mthreads_.end(); ++it) {
    (*it)->join();
    // delete *it;
  }
  mthreads_.clear();
  {
    std::unique_lock<mutex> lock(m_mutex);
    if (!mprequeue.empty()) {
      mprequeue.clear();
    }
    if (!mplayqueue.empty()) {
      mplayqueue.clear();
    }
  }
}

void REDThreadPool::thread_loop(std::string thread_name) {
#ifdef __APPLE__
  pthread_setname_np(thread_name.c_str());
#elif __ANDROID__
  pthread_setname_np(pthread_self(), thread_name.c_str());
#elif __HARMONY__
  pthread_setname_np(pthread_self(), thread_name.c_str());
#endif
  while (m_pool_start) {
    taskptr task = take_task();
    if (task == nullptr)
      continue;
    bool ret = task->run();
    AV_LOGW(LOG_TAG, "preload task %p complete %d\n", task.get(), ret);
  }
}

std::shared_ptr<REDDownLoadTask> REDThreadPool::take_task() {
  std::unique_lock<mutex> lock(m_mutex);
  while (mprequeue.empty() && m_pool_start) {
    m_cond.wait(lock);
  }

  taskptr task;
  if (!mprequeue.empty() && m_pool_start) {
    task = mprequeue.front();
    mprequeue.pop_front();
    AV_LOGW(LOG_TAG, "take_task %p  size %zu in waiting queue\n", task.get(),
            mprequeue.size());
  }

  return task;
}

void REDThreadPool::add_task(shared_ptr<REDDownLoadTask> task, bool bpreload) {
  std::unique_lock<mutex> lock(m_mutex);
  if (task == nullptr) {
    AV_LOGW(LOG_TAG, "add_task task is nullptr\n");
    return;
  }
  if (bpreload) {
    if (RedDownloadConfig::getinstance()->get_config_value(PRELOAD_LRU_KEY) ==
        1)
      mprequeue.push_front(task);
    else
      mprequeue.push_back(task);
    AV_LOGW(LOG_TAG, "add_task %p  size %zu in waiting queue\n", task.get(),
            mprequeue.size());
    m_cond.notify_one();
  } else {
    AV_LOGW(LOG_TAG,
            "add_task %p  size %zu create new thread for the running task\n",
            task.get(), mplayqueue.size());
    stoptask();
    task->asynctask();
    mplayqueue.push_back(task);
  }
}

void REDThreadPool::delete_task(shared_ptr<REDDownLoadTask> task) {
  std::unique_lock<mutex> lock(m_mutex);
  if (task == nullptr) {
    return;
  }
  if (std::find(mprequeue.begin(), mprequeue.end(), task) != mprequeue.end()) {
    mprequeue.remove(task);
  }
}

void REDThreadPool::move_task_to_head(shared_ptr<REDDownLoadTask> task) {
  std::unique_lock<mutex> lock(m_mutex);
  if (std::find(mprequeue.begin(), mprequeue.end(), task) != mprequeue.end()) {
    mprequeue.remove(task);
    mprequeue.push_front(task);
  }
}

void REDThreadPool::stoptask() {
  if (!mplayqueue.empty()) {
    for (auto iter = mplayqueue.begin(); iter != mplayqueue.end();) {
      if ((*iter)->iscompelte()) {
        mplayqueue.erase(iter);
      } else {
        ++iter;
      }
    }
  }
}

void REDThreadPool::clear(bool bpreload) {
  std::unique_lock<mutex> lock(m_mutex);
  if (bpreload) {
    mprequeue.clear();
  } else {
    mplayqueue.clear();
  }
}
