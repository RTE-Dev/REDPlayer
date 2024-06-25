#include "REDDownloadCacheManagerImpl.h"

#include "REDThreadPool.h"
#include "RedLog.h"
#include "dnscache/REDDnsCache.h"
#include <inttypes.h>

#ifdef ANDROID
#include <android/log.h>
#include <string.h>
#endif

#define LOG_TAG "RedDownloadCacheManager"
#define MAX_CACHE 10
#define MAX_QUEUE_SIZE 20

using namespace std;

std::once_flag RedDownloadCacheManager::RedDownloadCacheManagerOnceFlag;
RedDownloadCacheManager
    *RedDownloadCacheManager::RedDownloadCacheManagerInstance = nullptr;

RedDownloadCacheManager *RedDownloadCacheManager::getinstance() {
  std::call_once(RedDownloadCacheManagerOnceFlag, [&] {
    RedDownloadCacheManagerInstance =
        new (std::nothrow) RedDownloadCacheManagerImpl;
  });
  return RedDownloadCacheManagerInstance;
}

RedDownloadCacheManager::~RedDownloadCacheManager() {}

RedDownloadCacheManagerImpl::RedDownloadCacheManagerImpl() {
  minited = false;
  //    RedLogSetLevel(<#int level#>)(AV_LEVEL_INFO);
  // AV_LOGW(LOG_TAG, "%s init\n", __FUNCTION__);
}

RedDownloadCacheManagerImpl::~RedDownloadCacheManagerImpl() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (mveccache.size() > 0)
    mveccache.clear();
  if (mvecprecache.size() > 0)
    mvecprecache.clear();
  if (mvecadscache.size() > 0)
    mvecadscache.clear();

  REDFileManager::delInstance();
  // AV_LOGW(LOG_TAG, "%s destroy\n", __FUNCTION__);
}

void RedDownloadCacheManagerImpl::Global_init(DownLoadOpt *opt) {
  std::lock_guard<std::mutex> lock(m_mutex);
  // AV_LOGW(LOG_TAG, "%s, init reddownload \n", __FUNCTION__);
  if (opt != nullptr && !opt->cache_file_dir.empty()) {
    if (!minited) {
      if (RedDownloadConfig::getinstance()->get_config_value(USE_DNS_CACHE) ==
          1) {
        // REDDnsCache::getinstance()->SetHostDnsServerIp(opt->dnsserverip);
        int timeval = RedDownloadConfig::getinstance()->get_config_value(
            DNS_TIME_INTERVAL);
        if (timeval > 0)
          REDDnsCache::getinstance()->SetTimescale(timeval * 1000);
        REDDnsCache::getinstance()->run();
      }
      REDThreadPool *redpool = REDThreadPool::getinstance();
      redpool->initialize_threadpool(opt->threadpoolsize);
    }
    std::string url = "";
    RedDownloadCache *reddownload = new RedDownloadCache(url);
    reddownload->setOpt(opt);
    delete reddownload;
    reddownload = nullptr;
    minited = true;
  }
}

void RedDownloadCacheManagerImpl::getAllCachedFile(const std::string &dirpath,
                                                   char ***cached_file,
                                                   int *cached_file_len) {
  std::lock_guard<std::mutex> lock(m_file_mutex);
  std::string url = "";
  RedDownloadCache *reddownload = new RedDownloadCache(url);

  DownLoadOpt *opt = new DownLoadOpt();
  opt->cache_file_dir = dirpath;
  reddownload->setOpt(opt);
  std::string cache_path = dirpath;
  if (dirpath.back() != '/') {
    cache_path += '/';
  }
  reddownload->getAllCachedFile(
      cache_path, cached_file,
      cached_file_len); // todo(renwei) try no file, need to setOpt?
  delete reddownload;
  reddownload = nullptr;
}

void RedDownloadCacheManagerImpl::deleteCache(const std::string &dirpath,
                                              const std::string &uri,
                                              bool is_full_url) {
  std::lock_guard<std::mutex> lock(m_file_mutex);
  if (uri.empty()) {
    return;
  }

  std::string key = uri;
  if (is_full_url) {
    if (strncmp(uri.c_str(), "http", 4) != 0) {
      return;
    }
    UrlParser up(uri);
    key = up.geturi();
  }

  std::string url = "";
  RedDownloadCache *reddownload = new RedDownloadCache(url);
  DownLoadOpt *opt = new DownLoadOpt();
  opt->cache_file_dir = dirpath;
  reddownload->setOpt(opt);

  std::string cache_path = dirpath;
  if (dirpath.back() != '/') {
    cache_path += '/';
  }
  reddownload->deleteCache(cache_path, key);
  delete reddownload;
  reddownload = nullptr;
  AV_LOGI(LOG_TAG, "%s, path is %s, uri is %s\n", __FUNCTION__, dirpath.c_str(),
          uri.c_str());
}

std::string RedDownloadCacheManagerImpl::GetUrlMd5Path(const std::string &url) {
  string md5str;
  if (!url.empty()) {
    UrlParser up(url);
    md5str = up.geturi();
  }
  return md5str;
}

int64_t RedDownloadCacheManagerImpl::Open(const std::string &url,
                                          DownLoadListen *callback,
                                          DownLoadOpt *opt) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (url.empty() || (strncmp(url.c_str(), "http", 4) != 0) || !opt) {
    return -1;
  }
  if (!minited) {
    // AV_LOGW(LOG_TAG, "%s, init reddownload \n", __FUNCTION__);
    if (RedDownloadConfig::getinstance()->get_config_value(USE_DNS_CACHE) ==
        1) {
      REDDnsCache::getinstance()->run();
    }
    REDThreadPool *redpool = REDThreadPool::getinstance();
    redpool->initialize_threadpool(opt->threadpoolsize);
    minited = true;
  }

  UrlParser up(url);
  string uri = up.geturi();

  AV_LOGW(LOG_TAG, "%s, open url %s\n", __FUNCTION__, url.c_str());
  shared_ptr<RedDownloadCache> reddownload = nullptr;
  reddownload = addcache(url, opt);
  if (reddownload == nullptr) {
    AV_LOGW(LOG_TAG, "%s, null reddownload for open\n", __FUNCTION__);
    return ERROR(ENOMEM);
  }
  reddownload->setOpt(opt);
  return reddownload->Open(url, callback);
}

int RedDownloadCacheManagerImpl::Read(const std::string &url, int64_t uid,
                                      uint8_t *buf, size_t nbyte) {
  shared_ptr<RedDownloadCache> reddownload = getcache(url, uid);
  if (reddownload != nullptr)
    return reddownload->Read(buf, nbyte);
  AV_LOGW(LOG_TAG, "%s, null reddownload for Read\n", __FUNCTION__);
  return ERROR(EIO);
}

int64_t RedDownloadCacheManagerImpl::GetCacheSize(const std::string &url) {
  if (url.empty() || (strncmp(url.c_str(), "http", 4) != 0) || !minited) {
    return 0;
  }
  // AV_LOGI(LOG_TAG, "%s, url:%s\n", __FUNCTION__, url.c_str());
  // shared_ptr<RedDownloadCache> reddownload = getcache(url, 0);
  // if (reddownload == nullptr) {
  //    reddownload = make_shared<RedDownloadCache>(url);
  //}
  // return reddownload->GetRedDownloadSize(url);

  UrlParser up(url);
  string uri = up.geturi();
  int64_t cachesize = 0;
  if (!uri.empty()) {
    string path;
    cachesize = REDFileManager::getInstance()->get_cache_size(uri, path);
  }
  AV_LOGI(LOG_TAG, "%s, url:%s size %" PRId64 "\n", __FUNCTION__, url.c_str(),
          cachesize);
  return cachesize;
}

std::string
RedDownloadCacheManagerImpl::GetCacheFilePath(const std::string &path,
                                              const std::string &url) {
  std::string cache_path = path;
  if (cache_path.back() != '/') {
    cache_path += '/';
  }
  cache_path += GetUrlMd5Path(url);
  AV_LOGI(LOG_TAG, "%s, path is %s\n", __FUNCTION__, path.c_str());
  return cache_path;
}

int64_t RedDownloadCacheManagerImpl::GetCacheSizeByUri(const std::string &path,
                                                       const std::string &uri,
                                                       bool is_full_url) {
  std::lock_guard<std::mutex> lock(m_file_mutex);
  if (uri.empty()) {
    return 0;
  }

  std::string key = uri;
  if (is_full_url) {
    if (strncmp(uri.c_str(), "http", 4) != 0) {
      return 0;
    }
    UrlParser up(uri);
    key = up.geturi();
  }
  std::string cache_path = path;
  if (!path.empty() && path.back() != '/') {
    cache_path += '/';
  }

  std::string url = "";
  RedDownloadCache *reddownload = new RedDownloadCache(url);
  DownLoadOpt *opt = new DownLoadOpt();
  opt->cache_file_dir = path;
  reddownload->setOpt(opt);
  AV_LOGI(LOG_TAG, "%s, path:%s, uri:%s\n", __FUNCTION__, cache_path.c_str(),
          uri.c_str());

  int64_t cachesize = 0;
  if (!uri.empty()) {
    cachesize = REDFileManager::getInstance()->get_cache_size(key, cache_path);
  }
  AV_LOGI(LOG_TAG, "%s, url:%s size %" PRId64 "\n", __FUNCTION__, key.c_str(),
          cachesize);
  return cachesize;
}

/*
bool RedDownloadCacheManager::setOpt(const std::string &url, const string &key,
                             const string &value) {
    shared_ptr<RedDownloadCache> reddownload = getcache(url);
    //reddownload->setOpt(key, value);
    return true;
}*/
void RedDownloadCacheManagerImpl::Stop(const std::string &url, int64_t uid) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (url.empty()) {
    // stop all precache
    if (RedDownloadConfig::getinstance()->get_config_value(PRELOAD_LRU_KEY) ==
        0) {
      mvecprecache.clear();
    }
  } else {
    for (auto iter = mvecprecache.begin(); iter != mvecprecache.end(); ++iter) {
      if ((*iter)->Compareurl(url)) {
        (*iter)->Pause();
        break;
      }
    }
    // check cache vector
    // for (auto iter = mveccache.begin(); iter != mveccache.end(); ++iter)
    // {
    //     if ((*iter)->Compareurl(url)) {
    //         mveccache.erase(iter);
    //         break;
    //    }
    // }
  }
  return;
}

void RedDownloadCacheManagerImpl::Close(const std::string &url, int64_t uid) {
  // AV_LOGW(LOG_TAG, "%s begin\n", __FUNCTION__);
  shared_ptr<RedDownloadCache> reddownload = movcache(url, uid);
  // AV_LOGW(LOG_TAG, "%s end\n", __FUNCTION__);
}

int64_t RedDownloadCacheManagerImpl::Seek(const std::string &url, int64_t uid,
                                          int64_t offset, int whence) {
  shared_ptr<RedDownloadCache> reddownload = getcache(url, uid);
  if (reddownload != nullptr)
    return reddownload->Seek(offset, whence);
  AV_LOGW(LOG_TAG, "%s, null reddownload for Seek\n", __FUNCTION__);
  return offset;
}

shared_ptr<RedDownloadCache>
RedDownloadCacheManagerImpl::getcache(const std::string &url, int64_t uid) {
  std::lock_guard<std::mutex> lock(m_mutex);
  // check cache vector
  for (auto iter = mveccache.begin(); iter != mveccache.end(); ++iter) {
    if ((*iter)->Compareurl(url) && (uid == 0 || (*iter)->GetUid() == uid)) {
      return *iter;
    }
  }
  for (auto iter = mvecprecache.begin(); iter != mvecprecache.end(); ++iter) {
    if ((*iter)->Compareurl(url)) {
      return *iter;
    }
  }
  return nullptr;
}

shared_ptr<RedDownloadCache>
RedDownloadCacheManagerImpl::movcache(const std::string &url, int64_t uid) {
  shared_ptr<RedDownloadCache> reddownload = nullptr;
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto iter = mvecprecache.begin(); iter != mvecprecache.end(); ++iter) {
    if ((*iter)->Compareurl(url)) {
      reddownload = *iter;
      mvecprecache.erase(iter);
      break;
    }
  }
  // check cache vector
  for (auto iter = mveccache.begin(); iter != mveccache.end(); ++iter) {
    if ((*iter)->Compareurl(url) && (*iter)->GetUid() == uid) {
      reddownload = *iter;
      mveccache.erase(iter);
      break;
    }
  }

  for (auto iter = mvecadscache.begin(); iter != mvecadscache.end(); ++iter) {
    if ((*iter)->Compareurl(url)) {
      reddownload = *iter;
      mvecadscache.erase(iter);
      break;
    }
  }
  return reddownload;
}

shared_ptr<RedDownloadCache>
RedDownloadCacheManagerImpl::addcache(const std::string &url,
                                      DownLoadOpt *opt) {
  shared_ptr<RedDownloadCache> reddownload = nullptr;
  if (opt == nullptr) {
    return reddownload;
  }
  if (opt->DownLoadType == DOWNLOADADS) {
    for (auto iter = mvecadscache.begin(); iter != mvecadscache.end(); ++iter) {
      if ((*iter)->Compareurl(url)) {
        if (RedDownloadConfig::getinstance()->get_config_value(
                PRELOAD_LRU_KEY) == 0) {
          mvecadscache.erase(iter);
          break;
        } else if (RedDownloadConfig::getinstance()->get_config_value(
                       PRELOAD_LRU_KEY) == 2) {
          return nullptr;
        }
      }
    }
    if (reddownload == nullptr) {
      reddownload = make_shared<RedDownloadCache>(url);
    }
    if (mvecadscache.size() > MAX_CACHE) {
      auto iter = mvecadscache.begin();
      (*iter)->Close();
      mvecadscache.erase(iter);
    }
    mvecadscache.push_back(reddownload);
  } else if (opt->PreDownLoadSize <= 0) {
    // stop all precache
    REDThreadPool::getinstance()->clear(true);
    for (auto iter = mvecprecache.begin(); iter != mvecprecache.end();) {
      mvecprecache.erase(iter);
      AV_LOGI(LOG_TAG, "%s Close cache of precache\n", __FUNCTION__);
    }

    // check ads vector
    for (auto iter = mvecadscache.begin(); iter != mvecadscache.end(); ++iter) {
      if ((*iter)->Compareurl(url)) {
        // reddownload = *iter;
        (*iter)->Close();
        mvecadscache.erase(iter);
        break;
      }
    }
    if (reddownload == nullptr) {
      reddownload = make_shared<RedDownloadCache>(url);
    }
    if (mveccache.size() > MAX_CACHE) {
      auto iter = mveccache.begin();
      (*iter)->Close();
      mveccache.erase(iter);
    }
    mveccache.push_back(reddownload);
  } else {
    for (auto iter = mvecprecache.begin(); iter != mvecprecache.end(); ++iter) {
      if ((*iter)->Compareurl(url)) {
        if (RedDownloadConfig::getinstance()->get_config_value(
                PRELOAD_LRU_KEY) == 0) {
          mvecprecache.erase(iter);
          break;
        } else if (RedDownloadConfig::getinstance()->get_config_value(
                       PRELOAD_LRU_KEY) == 2) {
          return nullptr;
        } else {
          (*iter)->UpdatePreloadTaskPriority();
          return nullptr;
        }
      }
    }
    for (auto iter = mveccache.begin(); iter != mveccache.end(); ++iter) {
      if ((*iter)->Compareurl(url)) {
        if (RedDownloadConfig::getinstance()->get_config_value(
                PRELOAD_REOPEN_KEY) > 0) {
          AV_LOGI(LOG_TAG, "%s, create preload cache\n", __FUNCTION__);
          break;
        }
        AV_LOGE(LOG_TAG, "%s, get preload cache from normal cache\n",
                __FUNCTION__);
        return nullptr;
      }
    }
    if (reddownload == nullptr) {
      reddownload = make_shared<RedDownloadCache>(url);
    }
    if (mvecprecache.size() > MAX_CACHE) {
      auto iter = mvecprecache.begin();
      (*iter)->Close();
      mvecprecache.erase(iter);
    }
    mvecprecache.push_back(reddownload);
  }
  return reddownload;
}

void RedDownloadCacheManagerImpl::setDownloadCdn(const char *url,
                                                 int download_cdn) {
  // AV_LOGW(LOG_TAG, "%s begin\n", __FUNCTION__);
  shared_ptr<RedDownloadCache> reddownload = getcache(url, 0);
  if (reddownload != nullptr) {
    return reddownload->SetDownloadCdn(download_cdn);
  }
  // AV_LOGE(LOG_TAG, "%s, url %s, switch mode to %s Failed, not url found\n",
}
