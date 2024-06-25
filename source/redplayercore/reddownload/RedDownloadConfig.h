#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

// Public
#define THREADPOOL_SIZE_KEY "threadpool_size"
#define SHAREDNS_KEY "sharedns"
#define DOWNLOADCACHESIZE "downloadcachesize"
#define RETRY_COUNT "reddownloadretrycount"
#define PRELOAD_CLOSE "reddownloadpreclose"
#define PRELOAD_LRU_KEY "preload_lru"
#define OPEN_CURL_DEBUG_INFO_KEY "open_curl_debug_info"
#define USE_DNS_CACHE "use_dns_cache"
#define FORCE_USE_IPV6 "force_use_ipv6"
#define PARSE_NO_PING "parse_no_ping"
#define DNS_TIME_INTERVAL "dns_time_interval"
#define PRELOAD_REOPEN_KEY "preload_reopen"
#define FIX_GETDIR_SIZE "fix_getdir_size"
#define FIX_DATACALLBACK_CRASH_KEY "fix_datacallback_crash"
#define CALLBACK_DNS_INFO_KEY "callback_dns_info"
#define HTTPDNS_KEY "httpdns"
#define ABORT_KEY "abort"
#define NTECACHE_MBUF_SIZE "reddownload_mbuf_size"
#define HTTP_RANGE_SIZE "http_range_size"
#define IPRESOLVE_KEY "ip_resolve"
#define SHAREDNS_LIVE_KEY "sharedns_live"
#define IP_DOWNGRADE_KEY "ip_downgrade_live"
#define ENABLE_KUAISHOU_LOG "enable_kuaishou_log"
#define RANGE_SIZE_ONLY_CDN_KEY "range_size_only_cdn"
// Internal

class RedDownloadConfig final {
public:
  static RedDownloadConfig *getinstance();
  static std::once_flag redDownloadConfigOnceFlag;
  static RedDownloadConfig *redDownloadConfigInstance;
  void set_config(const std::string &key, int value);
  int get_config_value(const std::string &key);

  void set_internal_config(const std::string &key, int value);
  int get_internal_config_value(const std::string &key);

private:
  RedDownloadConfig();

private:
  std::unordered_map<std::string, int> config_map_;
  std::mutex config_mutex_;

  std::unordered_map<std::string, int> internal_config_map_;
  std::mutex internal_config_mutex_;
};
