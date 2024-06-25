#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include "REDDownloadListen.h"
#include "REDDownloaderBase.h"
#include "RedDownloadConfig.h"
#include "curl/curl.h"

#define MAX_CURL_DEBUG_LEN 4000

class RedCurl : public RedDownloadBase {
public:
  explicit RedCurl(Source source = Source::CDN, int range_size = 0);
  ~RedCurl() override;
  bool prepare();
  int filldata(RedDownLoadPara *downpara, size_t size) override;
  int rundownload(RedDownLoadPara *downpara) override;
  int precreatetask(RedDownLoadPara *downpara) override;
  uint64_t getlength() override;
  bool pause(bool block) override;
  bool resume() override;
  void readdns(RedDownLoadPara *downpara) override;
  int updatepara(RedDownLoadPara *downpara, bool neednotify) override;
  void continuedownload() override;
  void stop() override;

private:
  enum class DnsStatus : uint32_t { Waiting = 0, Success, Fail };

private:
  explicit RedCurl(int dummy);
  static RedCurl *se;
  static CURLSH *share_handle;
  static std::mutex sharehandlemutex;
  static pthread_rwlock_t m_dnsLock;
  static pthread_rwlock_t m_sslLock;
  static size_t writefunc(uint8_t *buffer, size_t size, size_t nmemb,
                          void *userdata);
  static size_t headerfunc(void *buffer, size_t size, size_t nmemb,
                           void *userdata);
  static void curlLock(CURL *handle, curl_lock_data data,
                       curl_lock_access laccess, void *useptr);
  static void curlUnlock(CURL *handle, curl_lock_data data, void *useptr);
  int debugfunc(CURL *handle, curl_infotype type, char *data, size_t size,
                void *userptr);
  static int sockopt_callback(void *clientp, curl_socket_t curlfd,
                              curlsocktype purpose);
  static curl_socket_t opensocket_callback(void *clientp, curlsocktype purpose,
                                           struct curl_sockaddr *address);
  bool initCurl();
  bool PerformCurl(int &curlres);
  bool destroyCurl();
  void updateCurl();
  void removeShareHandle();
  int getheadinfo();
  int gethttperror(int http_code);
  int gettcperror(int tcp_code);
  void HttpCallBack(int errcode);
  void tcpCallback(int errcode);
  void dnsCallback(struct curl_sockaddr *address, DnsStatus status);
  void reportNetworkQuality();
  void updateipaddr();
  bool mBDummy = false;
  int max_retry{0};
  CURLM *multi_handle = nullptr;
  CURL *mcurl = nullptr;
  struct curl_slist *mheaderList{nullptr};
  struct curl_slist *mhost{nullptr};
  string mhostname;
  string mcuripaddr;
  std::mutex m_mutexCurl;
  std::condition_variable m_condition;
  std::atomic_bool m_pause_timeout{false};
  bool brunning{false};
  uint64_t mfilelength{0};
  int mretrycount{0};
  bool bdescurl{false};
  int mserial{0};
  int mnotitystate{0};
  int m_range_size{0};
  std::atomic_int64_t m_starttime{0};
  std::atomic_int64_t m_endtime{0};
  char m_curldebuginfo[MAX_CURL_DEBUG_LEN]{0};
  std::mutex m_mutex_infinite_range;
  std::condition_variable m_con_infinite_range;
  bool m_islive{false};
  bool m_continue_infinite_range{false};
  bool m_is_report_http_open{false};
  bool m_callback_dns{false};
  DnsStatus m_dns_status{DnsStatus::Waiting};
  CURLcode m_curlcode{CURLE_OK};
  std::atomic_bool m_is_read_dns{false};

  int filldata_by_range_size(RedDownLoadPara *downpara, size_t size);
};
