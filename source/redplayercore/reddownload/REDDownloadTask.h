#pragma once

#include <stdint.h>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "REDDownloadListen.h"
#include "REDDownloader.h"
#include "REDDownloaderFactory.h"
using namespace std;

struct RequsetInfo {
  uint64_t range_start{0};
  uint64_t range_end{0};
};

class REDDownLoadTask {
public:
  REDDownLoadTask();
  ~REDDownLoadTask();

public:
  bool run();
  void flush();
  void pause(bool block);
  void resume();
  void stop();
  void destroy();
  void setparameter(RedDownLoadPara *downpara);
  RedDownLoadPara *getparameter();
  bool updatepara();
  bool isnextpara();
  bool iscompelte();
  void asynctask();

  int filldata(size_t size, bool &need_update_range);
  int syncupdatepara(bool neednotify);

  void continuetask();
  std::shared_ptr<RedDownloadStatus> getdownloadstatus();

  void setDownloadSource(bool download_from_cdn);
  int getDownloadSourceType();

private:
  void createdownload();
  void running_task_loop(std::string thread_name);
  RedDownLoad *downloadhandle = nullptr;
  RedDownLoadPara *mdownpara = nullptr;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::atomic_bool babort;
  std::atomic_bool bcomplete;
  std::atomic_bool bupdatepara;
  std::thread *mthread = nullptr;
  RequsetInfo m_requset_info;
  std::atomic_bool m_last_download_source_is_cdn{true};
  std::atomic_int m_no_need_reconnect_cdn{0}; // 1 return  2 check range
  std::atomic_bool m_download_from_cdn{true};
  int m_cdn_player_size = 0; //  should <= REDDOWNLOAD.mbuf_extra_size
  static const int BUFFER_SIZE = 64 * 1024;
  static const int MAX_BUFFER_SIZE = 10 * 1024 * 1024;
  CDNType m_cdn_type{CDNType::kNormalCDN};
  std::atomic_bool last_is_cdn{true};
  bool m_datacallback_crash{false};
};
