#pragma once
#include <sstream>
#include <string>

#include "REDDownloadListen.h"

#define CURLERROR(e) (-(e)-100000)
#define ERROR(e) (-(e))
#define REDTAG(a, b, c, d)                                                     \
  ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define REDERRTAG(a, b, c, d) (-static_cast<int>(REDTAG(a, b, c, d)))

#define ERROR_HTTP_BAD_REQUEST REDERRTAG(0xF8, '4', '0', '0')
#define ERROR_HTTP_UNAUTHORIZED REDERRTAG(0xF8, '4', '0', '1')
#define ERROR_HTTP_FORBIDDEN REDERRTAG(0xF8, '4', '0', '3')
#define ERROR_HTTP_NOT_FOUND REDERRTAG(0xF8, '4', '0', '4')
#define ERROR_HTTP_OTHER_4XX REDERRTAG(0xF8, '4', 'X', 'X')
#define ERROR_HTTP_SERVER_ERROR REDERRTAG(0xF8, '5', 'X', 'X')
#define ERROR_EOF REDERRTAG('E', 'O', 'F', ' ') ///< End of file

#define ERROR_TCP_CONNECT_TIMEOUT -1001
#define ERROR_TCP_READ_TIMEOUT -1002
#define ERROR_TCP_WRITE_TIMEOUT -1003
#define ERROR_DNS_PARSE_FAIL -1006

#define ERROR_TCP_PRE_DNS_PARSE -10001

struct RedDownLoadPara {
  uint64_t range_start{0};
  uint64_t range_end{0};
  uint64_t downloadsize{0};
  uint64_t filesize{0};
  int64_t serial{0};
  bool seekable{true};
  bool preload_finished{false};
  std::string url;
  std::string cdn_url;
  int rangesize{1024 * 1024};
  DataCallBack *mdatacb{nullptr};
  std::weak_ptr<DataCallBack> mdatacb_weak;
  DownLoadListen *mdownloadcb{nullptr};
  DownLoadOpt *mopt{nullptr};
};

typedef struct RedDownloadStatus {
  std::atomic_int httpcode{0};
  std::atomic_int64_t downloadsize{0};
  std::atomic_int errorcode{0};
  std::atomic_bool bnotify{false};
} RedDownloadStatus;

class RedDownLoad {
public:
  RedDownLoad();
  virtual ~RedDownLoad();
  virtual uint64_t getlength() = 0;
  virtual int filldata(RedDownLoadPara *mdownpara, size_t size) = 0;
  virtual int rundownload(RedDownLoadPara *mdownpara) = 0;
  virtual int precreatetask(RedDownLoadPara *downpara) = 0;
  virtual bool pause(bool block) = 0;
  virtual bool resume() = 0;
  virtual void readdns(RedDownLoadPara *mdownpara) = 0;
  virtual int updatepara(RedDownLoadPara *downpara, bool neednotify) = 0;
  virtual void continuedownload() = 0;
  virtual void stop() = 0;
  virtual std::shared_ptr<RedDownloadStatus> getdownloadstatus() = 0;
};
