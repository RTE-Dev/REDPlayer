#pragma once

#include <string>
#include <vector>
#define RED_EVENT_WILL_HTTP_OPEN 1 // AVAppHttpEvent
#define RED_EVENT_DID_HTTP_OPEN 2  // AVAppHttpEvent
#define RED_EVENT_WILL_HTTP_SEEK 3 // AVAppHttpEvent
#define RED_EVENT_DID_HTTP_SEEK 4  // AVAppHttpEvent
#define RED_EVENT_WILL_DNS_PARSE 5 // AVAppDNSEvent
#define RED_EVENT_DID_DNS_PARSE 6  // AVAppDNSEvent
#define RED_SPEED_CALL_BACK 7

#define RED_CTRL_WILL_TCP_OPEN 0x20001 // AVAppTcpIOControl
#define RED_CTRL_DID_TCP_OPEN 0x20002  // AVAppTcpIOControl

#define RED_EVENT_ASYNC_STATISTIC 0x11000  // AVAppAsyncStatistic
#define RED_EVENT_ASYNC_READ_SPEED 0x11001 // AVAppAsyncReadSpeed
#define RED_EVENT_IO_TRAFFIC 0x12204     // AVAppIOTraffic report player traffic
#define RED_EVENT_CACHE_STATISTIC 0x1003 //  IJKIOAPP_EVENT_CACHE_STATISTIC
// Everything released
#define RED_EVENT_DID_RELEASE 0x30000
// callback per fragment downloaded
#define RED_EVENT_DID_FRAGMENT_COMPLETE 0x30001 // report preload traffic
// Changing to next url after previous one failed
#define RED_EVENT_URL_CHANGE 0x30002
// Callback error
#define RED_EVENT_ERROR 0x30003
// report is preload
#define RED_EVENT_IS_PRELOAD 0x40003
// report first caton offset
#define RED_EVENT_FIRST_CATON_OFFSET 0x40004

using namespace std;

enum class Source : uint32_t {
  CDN = 0,
};

struct RedHttpEvent {
  void *obj;
  string url;
  int64_t offset;
  int error;
  int http_code;
  int64_t filesize;
  string wan_ip{""};
  int http_rtt{0}; // ms
  Source source{Source::CDN};
};

struct RedIOTraffic {
  void *obj;
  int bytes;
  string url;
  Source source{Source::CDN};
  int cached_size;
  string cache_path; // this is for ads
  int cache_size;
};

struct RedTcpIOControl {
  int error;
  int family;
  char *ip;
  int port;
  int fd;
  string url;
  int tcp_rtt{0}; // ms
  Source source{Source::CDN};
};

struct RedDnsInfo {
  string domain{""};
  string ip{""};
  int family{0};
  int port{0};
  int status{0}; // fail: 0, success: 1
};

struct RedCacheStatistic {
  int64_t cache_physical_pos;
  int64_t cache_file_forwards;
  int64_t cache_file_pos;
  int64_t cache_count_bytes;
  int64_t logical_file_size;
  int64_t cached_size;
};

struct RedUrlChangeEvent {
  std::string current_url{""};
  int http_code{0};
  std::string next_url{""};
};

struct RedErrorEvent {
  string url;
  int error;
  Source source{Source::CDN};
};

enum IPTTYPE { IP_NONE = 0, IP_INET4, IP_INET6 };

enum DownLoadType {
  DOWNLOADNONE = 0,
  DOWNLOADDNS = 1,
  DOWNLOADPRE = 2,
  DOWNLOADDATA = 3,
  DOWNLOADPIC = 4,
  DOWNLOADADS = 5,
  DOWNLOADKAIPINGADS = 6,
};
class DownLoadListen {
public:
  virtual ~DownLoadListen() = default;
  virtual void DownLoadCb(int msg, void *arg1, void *arg2) = 0;
  virtual void DownLoadIoCb(int msg, void *arg1, void *arg2) = 0;
  virtual void UrlSelectCb(const char *oldUrl, int reconnectType, char *outUrl,
                           int outLen) = 0;
  virtual void SpeedCb(int64_t size, int64_t speed, int64_t cur_time) = 0;
  virtual int DnsCb(char *dnsip, int len) = 0;
  virtual int InterruptCb() = 0;
  virtual void *Getappctx() = 0;
  //   private:
  //     void *appctx{nullptr};
  //     void *ioappctx{nullptr};
};
class DataCallBack {
public:
  virtual ~DataCallBack() = default;
  virtual size_t WriteData(uint8_t *ptr, size_t size, void *userdata,
                           int mserial, int err_code = 0) = 0;
  virtual void DownloadCallBack(int what, void *arg1, void *arg2, int64_t arg3,
                                int64_t arg4) = 0;
  virtual int InterruptCallBack() = 0;
};

struct DownLoadOpt {
  int DownLoadType{DOWNLOADNONE};
  int64_t PreDownLoadSize{0};
  string cache_file_dir;
  uint32_t cache_max_entries{20};
  int64_t cache_max_dir_capacity{300 * 1024 * 1024};
  int iptype{0}; // 0 default 1 ipv4 2 ipv6
  int64_t dns_cache_timeout{3600000000};
  std::vector<string> headers;
  string useragent{"RedDownload"};
  string http_proxy;
  string referer;
  string dnsserverip;
  int max_retry{5};
  int low_speed_limit{1};       // 0-1
  int low_speed_time{3000};     // ms
  int connect_timeout{3000000}; // micro-second
  int showprogress{0};
  int threadpoolsize{1};
  bool readasync{true};
  bool loadfile{true};
  int tcp_buffer{-1}; // bytes
  bool islive{false};
  int cdn_player_size{0};
  int dns_timeout{0};
  string url_list_separator{""}; // if empty, not support url list; if not
                                 // empty, url is a list splitted by a separator
  bool use_https{false};
  std::string token; // for decrypt data
  int video_type;    // 1:first video, 2: related video
  char *exp_id;
  int cache_size{0};
};
