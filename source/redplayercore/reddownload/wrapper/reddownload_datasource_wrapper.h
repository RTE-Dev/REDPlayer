#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
typedef void (*AppEventCbWrapper)(void *, int, void *, void *);
typedef void (*UrlSelectCbWrapper)(void *, const char *, int, char *, int);
typedef void (*SpeedCbWrapper)(void *, int64_t, int64_t, int64_t);
typedef int (*InterruptWrapper)(void *);
typedef int (*DnsServerCbWrapper)(void *, char *, int);

#define RED_EVENT_WILL_HTTP_OPEN_W 1 // AVAppHttpEvent
#define RED_EVENT_DID_HTTP_OPEN_W 2  // AVAppHttpEvent
#define RED_EVENT_WILL_HTTP_SEEK_W 3 // AVAppHttpEvent
#define RED_EVENT_DID_HTTP_SEEK_W 4  // AVAppHttpEvent
#define RED_EVENT_WILL_DNS_PARSE_W 5 // AVAppDNSEvent
#define RED_EVENT_DID_DNS_PARSE_W 6  // AVAppDNSEvent
#define RED_EVENT_URL_CHANGE_W 7     // AVAPP_EVENT_URL_CHANGE

#define RED_CTRL_WILL_TCP_OPEN_W 0x20001 // AVAppTcpIOControl
#define RED_CTRL_DID_TCP_OPEN_W 0x20002  // AVAppTcpIOControl

#define RED_EVENT_ASYNC_STATISTIC_W 0x11000  // AVAppAsyncStatistic
#define RED_EVENT_ASYNC_READ_SPEED_W 0x11001 // AVAppAsyncReadSpeed
#define RED_EVENT_IO_TRAFFIC_W 0x12204       // AVAppIOTraffic
#define RED_EVENT_CACHE_STATISTIC_W 0x1003
#define RED_EVENT_DID_RELEASE_W 0x30000
#define RED_EVENT_DID_FRAGMENT_COMPLETE_W 0x30001
#define RED_EVENT_ERROR_W 0x30002

typedef struct RedHttpEventWrapper_ {
  void *obj;
  char url[4096];
  int64_t offset;
  int error;
  int http_code;
  int64_t filesize;
  char wan_ip[96];
  int http_rtt; // ms
  int source;
} RedHttpEventWrapper;

typedef struct RedTcpIOControlWrapper_ {
  int error;
  int family;
  char ip[96];
  int port;
  int fd;
  char url[4096];
  int tcp_rtt; // ms
  int source;
} RedTcpIOControlWrapper;

typedef struct RedDnsInfoWrapper_ {
  char domain[50];
  char ip[96];
  int family;
  int port;
  int status; // fail: 0, success: 1
} RedDnsInfoWrapper;

typedef struct RedIOTrafficWrapper_ {
  void *obj;
  int bytes;
  char url[4096];
  char cache_path[4096];
  int cache_size;
  int source;
} RedIOTrafficWrapper;

typedef struct RedCacheStatisticWrapper_ {
  int64_t cache_physical_pos;
  int64_t cache_file_forwards;
  int64_t cache_file_pos;
  int64_t cache_count_bytes;
  int64_t logical_file_size;
  int64_t cached_size;
} RedCacheStatisticWrapper;

typedef struct RedUrlChangeEventWrapper_ {
  char current_url[4096];
  int http_code;
  char next_url[4096];
} RedUrlChangeEventWrapper;

typedef struct RedErrorEventWrapper_ {
  char url[4096];
  int error;
  int source;
} RedErrorEventWrapper;

typedef struct DownLoadCbWrapper_ {
  AppEventCbWrapper appcb;
  AppEventCbWrapper ioappcb;
  UrlSelectCbWrapper urlcb;
  SpeedCbWrapper speedcb;
  InterruptWrapper interruptcb;
  DnsServerCbWrapper dnscb;
  void *appctx;
  void *ioappctx;
} DownLoadCbWrapper;

typedef struct DownLoadOptWrapper_ {
  int DownLoadType;
  int64_t PreDownLoadSize;
  const char *cache_file_dir;
  int64_t cache_max_dir_capacity;
  uint32_t cache_max_entries;
  int threadpool_size;
  int iptype; // 0 default 1 ipv4 2 ipv6
  int64_t dns_cache_timeout;
  const char *headers;
  const char *useragent;
  const char *http_proxy;
  const char *referer;
  const char *dnsserverip;
  int max_retry;
  int low_speed_limit; // 0-1
  int low_speed_time;  // ms
  int connect_timeout; // ms
  int readasync;
  int loadfile;
  int tcp_buffer; // bytes
  int dns_timeout;
  const char
      *url_list_separator; // if empty, not support url list; if not empty,
                           // url is a list splitted by a separator
  int use_https;
} DownLoadOptWrapper;

void reddownload_datasource_wrapper_log_set_back(void *func, void *arg);
int64_t reddownload_datasource_wrapper_open(const char *url,
                                            DownLoadCbWrapper *cb,
                                            DownLoadOptWrapper *opt);
void reddownload_datasource_wrapper_dnscb(DownLoadCbWrapper *cb);
void reddownload_datasource_wrapper_net_changed();
int reddownload_datasource_wrapper_read(const char *url, int64_t uid,
                                        unsigned char *buf, int size);
int64_t reddownload_datasource_wrapper_seek(const char *url, int64_t uid,
                                            int64_t pos, int whence);
int reddownload_datasource_wrapper_close(const char *url, int64_t uid);
void reddownload_datasource_wrapper_stop(const char *url, int64_t uid);
void reddownload_datasource_wrapper_opt_reset(DownLoadOptWrapper *opt);
void reddownload_datasource_wrapper_init(DownLoadOptWrapper *opt);
int64_t reddownload_datasource_wrapper_cache_size(const char *url);

void reddownload_global_config_set(const char *key, int value);
int reddownload_get_url_md5(const char *url, char *urlmd5, int Md5Length);

void reddownload_get_network_quality(int *level, int *speed);
void reddownload_datasource_wrapper_setdns(const char *dnsip);

void reddownload_set_download_cdn_wrapper(const char *url, int download_cdn);

int64_t reddownload_datasource_wrapper_cache_size_by_uri(const char *path,
                                                         const char *uri,
                                                         int is_full_url);
int reddownload_get_cache_file_path(const char *path, const char *url,
                                    char **file_path);
void reddownload_get_all_cached_file(const char *dirpath, char ***cached_file,
                                     int *cached_file_len);
int reddownload_delete_cache(const char *dirpath, const char *uri,
                             int is_full_url);
int reddownload_get_key(const char *url, char **file_key);
#ifdef __cplusplus
}
#endif
