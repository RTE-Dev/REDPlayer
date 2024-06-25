
#include "REDDownloadTask.h"

#include "RedDownloadConfig.h"
#include "RedLog.h"

#define LOG_TAG "RedDownloadTask"

REDDownLoadTask::REDDownLoadTask() : m_mutex(), m_cond() {
  babort.store(false);
  bupdatepara.store(false);
  bcomplete.store(false);
  m_datacallback_crash = RedDownloadConfig::getinstance()->get_config_value(
                             FIX_DATACALLBACK_CRASH_KEY) > 0;
}

REDDownLoadTask::~REDDownLoadTask() {
  destroy();
  if (mthread != nullptr && mthread->joinable()) {
    mthread->join();
    delete mthread;
    mthread = nullptr;
  }
  if (downloadhandle != nullptr)
    delete downloadhandle;
  downloadhandle = nullptr;
  if (mdownpara != nullptr)
    delete mdownpara;
  mdownpara = nullptr;
}

void REDDownLoadTask::asynctask() {
  mthread = new std::thread(std::bind(&REDDownLoadTask::running_task_loop, this,
                                      "REDDownloadThread"));
}

void REDDownLoadTask::running_task_loop(std::string thread_name) {
#ifdef __APPLE__
  pthread_setname_np(thread_name.c_str());
#elif __ANDROID__
  pthread_setname_np(pthread_self(), thread_name.c_str());
#elif __HARMONY__
  pthread_setname_np(pthread_self(), thread_name.c_str());
#endif

  do {
    bool ret = run();
    AV_LOGW(LOG_TAG, "running task %p, fragment complete %d\n", this, ret);
  } while (isnextpara());
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    bcomplete.store(true);
    AV_LOGW(LOG_TAG, "running task %p complete\n", this);
  }
}

bool REDDownLoadTask::run() {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (babort.load()) {
      return false;
    }
  }

  m_requset_info.range_start = mdownpara->range_start;
  m_requset_info.range_end = mdownpara->range_end;
  mdownpara->cdn_url = mdownpara->url;

  int ret = downloadhandle->rundownload(mdownpara);
  if (ret != 0 && ret != EOF) {
    AV_LOGI(LOG_TAG, "REDDownLoadTask %p %s ret %d\n", this, __FUNCTION__, ret);
    return false;
  }

  return true;
}

void REDDownLoadTask::flush() {
  if (downloadhandle != nullptr) {
    downloadhandle->pause(true);
    downloadhandle->resume();
  }
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    bupdatepara = false;
  }
}

void REDDownLoadTask::pause(bool block) {
  if (downloadhandle != nullptr)
    downloadhandle->pause(block);
}

bool REDDownLoadTask::iscompelte() {
  std::unique_lock<std::mutex> lock(m_mutex);
  return bcomplete.load();
}

void REDDownLoadTask::stop() {
  std::unique_lock<std::mutex> lock(m_mutex);
  babort.store(true);
  if (downloadhandle != nullptr)
    downloadhandle->pause(true);
  m_cond.notify_all();
}

void REDDownLoadTask::resume() {
  if (downloadhandle != nullptr)
    downloadhandle->resume();
}

void REDDownLoadTask::destroy() {
  std::unique_lock<std::mutex> lock(m_mutex);
  AV_LOGI(LOG_TAG, "REDDownLoadTask %p %s\n", this, __FUNCTION__);
  m_cond.notify_one();
}

int REDDownLoadTask::filldata(size_t size, bool &need_update_range) {
  if (downloadhandle == nullptr || mdownpara == nullptr) {
    AV_LOGI(LOG_TAG, "%p %s is null handle %p  para %p\n", this, __FUNCTION__,
            downloadhandle, mdownpara);
    return ERROR(EIO);
  }

  int ret = downloadhandle->filldata(mdownpara, size);
  if (m_cdn_player_size > 0 &&
      (mdownpara->downloadsize == mdownpara->range_end + 1)) {
    AV_LOGI(LOG_TAG, "%p %s update download range\n", this, __FUNCTION__);
    need_update_range = true;
  }
  return ret;
}

void REDDownLoadTask::setDownloadSource(bool download_from_cdn) {
  m_download_from_cdn.store(download_from_cdn);
}

int REDDownLoadTask::getDownloadSourceType() {
  return m_download_from_cdn.load() ? 0 : static_cast<int>(m_cdn_type);
}

int REDDownLoadTask::syncupdatepara(bool neednotify) {
  if (downloadhandle != nullptr) {
    return downloadhandle->updatepara(mdownpara, neednotify);
  }
  return 0;
}

void REDDownLoadTask::setparameter(RedDownLoadPara *downpara) {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (mdownpara == nullptr)
    mdownpara = downpara;

  if (mdownpara->mopt->cdn_player_size > 0 &&
      mdownpara->mopt->cdn_player_size <= MAX_BUFFER_SIZE) {
    m_cdn_player_size = mdownpara->mopt->cdn_player_size;
  } else {
    m_cdn_player_size = 0;
  }

  if (downloadhandle == nullptr && mdownpara != nullptr) {
    downloadhandle = RedDownLoadFactory::create(mdownpara, CDNType::kNormalCDN,
                                                m_cdn_player_size);
  }

  mdownpara->cdn_url = mdownpara->url;
}

bool REDDownLoadTask::updatepara() {
  std::unique_lock<std::mutex> lock(m_mutex);
  bupdatepara = true;
  m_cond.notify_one();
  return true;
}

bool REDDownLoadTask::isnextpara() {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (babort.load()) {
    AV_LOGI(LOG_TAG, "REDDownLoadTask  %p %s updatepara abort\n", this,
            __FUNCTION__);
    return false;
  } else if (bupdatepara) {
    bupdatepara = false;
    AV_LOGI(LOG_TAG, "REDDownLoadTask  %p %s updatepara true\n", this,
            __FUNCTION__);
  } else {
    m_cond.wait(lock);
    bupdatepara = false;
  }
  AV_LOGI(LOG_TAG, "REDDownLoadTask  %p %s abort %d\n", this, __FUNCTION__,
          babort.load());
  return !babort;
}

void REDDownLoadTask::createdownload() {
  downloadhandle = RedDownLoadFactory::create(mdownpara);
}

RedDownLoadPara *REDDownLoadTask::getparameter() { return mdownpara; }

void REDDownLoadTask::continuetask() {
  if (downloadhandle == nullptr) {
    AV_LOGE(LOG_TAG, "%p %s is null handle %p\n", this, __FUNCTION__,
            downloadhandle);
    return;
  }
  downloadhandle->continuedownload();
}

std::shared_ptr<RedDownloadStatus> REDDownLoadTask::getdownloadstatus() {
  if (downloadhandle == nullptr) {
    AV_LOGE(LOG_TAG, "%p %s is null handle %p\n", this, __FUNCTION__,
            downloadhandle);
    return nullptr;
  }
  return downloadhandle->getdownloadstatus();
}
