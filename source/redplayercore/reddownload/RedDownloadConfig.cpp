#include "RedDownloadConfig.h"

std::once_flag RedDownloadConfig::redDownloadConfigOnceFlag;
RedDownloadConfig *RedDownloadConfig::redDownloadConfigInstance = nullptr;
RedDownloadConfig *RedDownloadConfig::getinstance() {
  std::call_once(redDownloadConfigOnceFlag, [&] {
    redDownloadConfigInstance = new (std::nothrow) RedDownloadConfig;
  });
  return redDownloadConfigInstance;
}

RedDownloadConfig::RedDownloadConfig() {
  config_map_ = {
      {THREADPOOL_SIZE_KEY, 1},
      {SHAREDNS_KEY, 1},
      {DOWNLOADCACHESIZE, 1024 * 1024},
      {RETRY_COUNT, 3},
      {PRELOAD_CLOSE, 0},
      {PRELOAD_LRU_KEY, 0},
      {OPEN_CURL_DEBUG_INFO_KEY, 1},
      {USE_DNS_CACHE, 0},
      {FORCE_USE_IPV6, 0},
      {PARSE_NO_PING, 0},
      {DNS_TIME_INTERVAL, 0},
      {PRELOAD_REOPEN_KEY, 0},
      {FIX_GETDIR_SIZE, 0},
      {FIX_DATACALLBACK_CRASH_KEY, 0},
      {CALLBACK_DNS_INFO_KEY, 0},
      {HTTPDNS_KEY, 0},
      {ABORT_KEY, 0},
      {IPRESOLVE_KEY, 0},
      {SHAREDNS_LIVE_KEY, 0},
      {IP_DOWNGRADE_KEY, 0},
      {RANGE_SIZE_ONLY_CDN_KEY, 0},
  };
  internal_config_map_ = {};
}

void RedDownloadConfig::set_config(const std::string &key, int value) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  config_map_[key] = value;
  // AV_LOGI(LOG_TAG, "%s \"%s\":%d\n", __FUNCTION__, key.c_str(), value);
}

int RedDownloadConfig::get_config_value(const std::string &key) {
  std::lock_guard<std::mutex> lock(config_mutex_);
  if (config_map_.find(key) != config_map_.end()) {
    return config_map_[key];
  }
  return INT_MIN;
}

void RedDownloadConfig::set_internal_config(const std::string &key, int value) {
  std::lock_guard<std::mutex> lock(internal_config_mutex_);
  internal_config_map_[key] = value;
  // AV_LOGI(LOG_TAG, "%s \"%s\":%d\n", __FUNCTION__, key.c_str(), value);
}

int RedDownloadConfig::get_internal_config_value(const std::string &key) {
  std::lock_guard<std::mutex> lock(internal_config_mutex_);
  if (internal_config_map_.find(key) != internal_config_map_.end()) {
    return internal_config_map_[key];
  }
  return INT_MIN;
}
