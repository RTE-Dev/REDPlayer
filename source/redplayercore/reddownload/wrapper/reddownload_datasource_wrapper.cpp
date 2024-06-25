
#include "reddownload_datasource_wrapper.h"

#include "NetworkQuality.h"
#include "REDDownloadCacheManager.h"
#include "REDDownloader.h"
#include "RedLog.h"
#include "dnscache/REDDnsCache.h"

class DownLoadCbImp : public DownLoadListen {
public:
  explicit DownLoadCbImp(DownLoadCbWrapper *cb_) {
    this->cb = new DownLoadCbWrapper();
    if (cb_) {
      this->cb->appcb = cb_->appcb;
      this->cb->ioappcb = cb_->ioappcb;
      this->cb->urlcb = cb_->urlcb;
      this->cb->speedcb = cb_->speedcb;
      this->cb->interruptcb = cb_->interruptcb;
      this->cb->dnscb = cb_->dnscb;
      this->cb->appctx = cb_->appctx;
      this->cb->ioappctx = cb_->ioappctx;
    }
  }

  DownLoadCbImp() { this->cb = nullptr; }

  ~DownLoadCbImp() {
    if (this->cb) {
      delete this->cb;
      this->cb = nullptr;
    }
  }

  void *Getappctx() {
    if (this->cb != nullptr) {
      return this->cb->appctx;
    }
    return nullptr;
  }

  void DownLoadCb(int msg, void *arg1, void *arg2) {
    if (!this->cb || !this->cb->appcb) {
      return;
    }
    int msg_w = 0;
    void *arg_w = NULL;
    switch (msg) {
    case RED_EVENT_WILL_HTTP_OPEN:
      msg_w = RED_EVENT_WILL_HTTP_OPEN_W;
      break;
    case RED_EVENT_DID_HTTP_OPEN: {
      msg_w = RED_EVENT_DID_HTTP_OPEN_W;
      RedHttpEventWrapper *arg1_w = reinterpret_cast<RedHttpEventWrapper *>(
          malloc(sizeof(RedHttpEventWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedHttpEventWrapper));
      if (arg1_w && arg1) {
        RedHttpEvent *data = reinterpret_cast<RedHttpEvent *>(arg1);
        arg1_w->error = data->error;
        arg1_w->http_code = data->http_code;
        arg1_w->filesize = data->filesize;
        if (!data->url.empty() && data->url.size() > 0 &&
            data->url.size() < sizeof(arg1_w->url)) {
          memcpy(arg1_w->url, data->url.c_str(), data->url.size());
        }
        arg1_w->offset = data->offset;
        if (!data->wan_ip.empty() && data->wan_ip.size() > 0 &&
            data->wan_ip.size() < sizeof(arg1_w->wan_ip)) {
          memcpy(arg1_w->wan_ip, data->wan_ip.c_str(), data->wan_ip.size());
        }
        arg1_w->http_rtt = data->http_rtt;
        arg1_w->source = static_cast<int>(data->source);
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_WILL_HTTP_SEEK:
      msg_w = RED_EVENT_WILL_HTTP_SEEK_W;
      break;
    case RED_EVENT_DID_HTTP_SEEK:
      msg_w = RED_EVENT_DID_HTTP_SEEK_W;
      break;
    case RED_EVENT_WILL_DNS_PARSE: {
      msg_w = RED_EVENT_WILL_DNS_PARSE_W;
      RedDnsInfoWrapper *arg1_w = reinterpret_cast<RedDnsInfoWrapper *>(
          malloc(sizeof(RedDnsInfoWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedDnsInfoWrapper));
      if (arg1_w && arg1) {
        RedDnsInfo *data = reinterpret_cast<RedDnsInfo *>(arg1);
        if (!data->domain.empty())
          strlcpy(arg1_w->domain, data->domain.c_str(), sizeof(arg1_w->domain));
        if (!data->ip.empty())
          strlcpy(arg1_w->ip, data->ip.c_str(), sizeof(arg1_w->ip));
        arg1_w->family = data->family;
        arg1_w->port = data->port;
        arg1_w->status = data->status;
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_DID_DNS_PARSE: {
      msg_w = RED_EVENT_DID_DNS_PARSE_W;
      RedDnsInfoWrapper *arg1_w = reinterpret_cast<RedDnsInfoWrapper *>(
          malloc(sizeof(RedDnsInfoWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedDnsInfoWrapper));
      if (arg1_w && arg1) {
        RedDnsInfo *data = reinterpret_cast<RedDnsInfo *>(arg1);
        if (!data->domain.empty())
          strlcpy(arg1_w->domain, data->domain.c_str(), sizeof(arg1_w->domain));
        if (!data->ip.empty())
          strlcpy(arg1_w->ip, data->ip.c_str(), sizeof(arg1_w->ip));
        arg1_w->family = data->family;
        arg1_w->port = data->port;
        arg1_w->status = data->status;
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_CTRL_WILL_TCP_OPEN: {
      msg_w = RED_CTRL_WILL_TCP_OPEN_W;
      if (arg1) {
        int *arg1_w = reinterpret_cast<int *>(malloc(sizeof(int)));
        if (arg1_w) {
          memcpy(arg1_w, arg1, sizeof(int));
        }
        arg_w = reinterpret_cast<void *>(arg1_w);
      }
      break;
    }
    case RED_CTRL_DID_TCP_OPEN: {
      msg_w = RED_CTRL_DID_TCP_OPEN_W;
      RedTcpIOControlWrapper *arg1_w =
          reinterpret_cast<RedTcpIOControlWrapper *>(
              malloc(sizeof(RedTcpIOControlWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedTcpIOControlWrapper));
      if (arg1_w && arg1) {
        RedTcpIOControl *data = reinterpret_cast<RedTcpIOControl *>(arg1);
        arg1_w->error = data->error;
        arg1_w->family = data->family;
        if (data->ip) {
          memcpy(arg1_w->ip, data->ip, strlen(data->ip));
        }
        arg1_w->port = data->port;
        arg1_w->fd = data->fd;
        if (!data->url.empty() && data->url.size() > 0 &&
            data->url.size() < sizeof(arg1_w->url)) {
          memcpy(arg1_w->url, data->url.c_str(), data->url.size());
        }
        arg1_w->tcp_rtt = data->tcp_rtt;
        arg1_w->source = static_cast<int>(data->source);
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_ASYNC_STATISTIC:
      msg_w = RED_EVENT_ASYNC_STATISTIC_W;
      break;
    case RED_EVENT_ASYNC_READ_SPEED:
      msg_w = RED_EVENT_ASYNC_READ_SPEED_W;
      break;
    case RED_EVENT_IO_TRAFFIC: {
      msg_w = RED_EVENT_IO_TRAFFIC_W;
      RedIOTrafficWrapper *arg1_w = reinterpret_cast<RedIOTrafficWrapper *>(
          malloc(sizeof(RedIOTrafficWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedIOTrafficWrapper));
      if (arg1_w && arg1) {
        RedIOTraffic *data = reinterpret_cast<RedIOTraffic *>(arg1);
        arg1_w->bytes = data->bytes;
        if (!data->url.empty() && data->url.size() > 0 &&
            data->url.size() < sizeof(arg1_w->url)) {
          memcpy(arg1_w->url, data->url.c_str(), data->url.size());
        }
        arg1_w->source = static_cast<int>(data->source);
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_CACHE_STATISTIC: {
      //            AV_LOGW(NULL, "lpp_redcul wrappr\n");
      msg_w = RED_EVENT_CACHE_STATISTIC_W;
      RedCacheStatisticWrapper *arg1_w =
          reinterpret_cast<RedCacheStatisticWrapper *>(
              malloc(sizeof(RedCacheStatisticWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedCacheStatisticWrapper));
      if (arg1_w && arg1) {
        RedCacheStatistic *data = reinterpret_cast<RedCacheStatistic *>(arg1);
        arg1_w->cached_size = data->cached_size;
        arg1_w->logical_file_size = data->logical_file_size;
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_DID_RELEASE:
      msg_w = RED_EVENT_DID_RELEASE_W;
      break;
    case RED_EVENT_DID_FRAGMENT_COMPLETE: {
      msg_w = RED_EVENT_DID_FRAGMENT_COMPLETE_W;
      RedIOTrafficWrapper *arg1_w = reinterpret_cast<RedIOTrafficWrapper *>(
          malloc(sizeof(RedIOTrafficWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedIOTrafficWrapper));
      if (arg1_w && arg1) {
        RedIOTraffic *data = reinterpret_cast<RedIOTraffic *>(arg1);
        arg1_w->bytes = data->bytes;
        if (!data->url.empty() && data->url.size() > 0 &&
            data->url.size() < sizeof(arg1_w->url)) {
          memcpy(arg1_w->url, data->url.c_str(), data->url.size());
        }
        arg1_w->source = static_cast<int>(data->source);
        arg1_w->cache_size = data->cache_size;
        if (!data->cache_path.empty() && data->cache_path.size() > 0 &&
            data->cache_path.size() < sizeof(arg1_w->cache_path)) {
          memcpy(arg1_w->cache_path, data->cache_path.c_str(),
                 data->cache_path.size());
        }
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_URL_CHANGE: {
      msg_w = RED_EVENT_URL_CHANGE_W;
      RedUrlChangeEventWrapper *arg1_w =
          reinterpret_cast<RedUrlChangeEventWrapper *>(
              malloc(sizeof(RedUrlChangeEventWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedUrlChangeEventWrapper));
      if (arg1_w && arg1) {
        RedUrlChangeEvent *data = reinterpret_cast<RedUrlChangeEvent *>(arg1);
        if (!data->current_url.empty() && data->current_url.size() > 0 &&
            data->current_url.size() < sizeof(arg1_w->current_url)) {
          memcpy(arg1_w->current_url, data->current_url.c_str(),
                 data->current_url.size());
        }
        arg1_w->http_code = data->http_code;
        if (!data->next_url.empty() && data->next_url.size() > 0 &&
            data->next_url.size() < sizeof(arg1_w->next_url)) {
          memcpy(arg1_w->next_url, data->next_url.c_str(),
                 data->next_url.size());
        }
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    case RED_EVENT_ERROR: {
      msg_w = RED_EVENT_ERROR_W;
      RedErrorEventWrapper *arg1_w = reinterpret_cast<RedErrorEventWrapper *>(
          malloc(sizeof(RedErrorEventWrapper)));
      if (arg1_w)
        memset(arg1_w, 0, sizeof(RedErrorEventWrapper));
      if (arg1_w && arg1) {
        RedErrorEvent *data = reinterpret_cast<RedErrorEvent *>(arg1);
        if (!data->url.empty() && data->url.size() > 0 &&
            data->url.size() < sizeof(arg1_w->url)) {
          memcpy(arg1_w->url, data->url.c_str(), data->url.size());
        }
        arg1_w->error = data->error;
        arg1_w->source = static_cast<int>(data->source);
      }
      arg_w = reinterpret_cast<void *>(arg1_w);
      break;
    }
    default:
      break;
    }
    this->cb->appcb(this->cb->appctx, msg_w, arg_w, arg2);
    if (arg_w) {
      free(arg_w);
      arg_w = NULL;
    }
  }

  // 暂时底层没调用
  void DownLoadIoCb(int msg, void *arg1, void *arg2) {
    if (this->cb != nullptr && this->cb->ioappcb != nullptr) {
      // this->cb->ioappcb(this->cb->ioappctx,msg,arg1,arg2);
    }
  }

  // 暂时底层没调用
  void UrlSelectCb(const char *oldUrl, int reconnectType, char *outUrl,
                   int outLen) {
    if (this->cb != nullptr && this->cb->urlcb != nullptr) {
      this->cb->urlcb(this->cb->ioappctx, oldUrl, reconnectType, outUrl,
                      outLen);
    }
  }

  int DnsCb(char *dnsip, int outLen) {
    if (this->cb != nullptr && this->cb->dnscb != nullptr) {
      return this->cb->dnscb(this->cb->appctx, dnsip, outLen);
    }
    return 0;
  }

  void SpeedCb(int64_t size, int64_t speed, int64_t cur_time) {
    if (this->cb != nullptr && this->cb->speedcb != nullptr) {
      // AV_LOGI(LOG_TAG, "%s, size %lld, speed %lld, time %lld",
      // __FUNCTION__, size, speed, cur_time);
      this->cb->speedcb(this->cb->appctx, size, speed, cur_time);
    }
  }

  int InterruptCb() {
    if (this->cb != nullptr && this->cb->interruptcb != nullptr) {
      return this->cb->interruptcb(this->cb->ioappctx);
    }
    return 0;
  }

private:
  DownLoadCbWrapper *cb{nullptr};
};

void reddownload_datasource_wrapper_setdns(const char *dnsip) {
  if (dnsip == nullptr)
    return;
  REDDnsCache::getinstance()->SetHostDnsServerIp(dnsip);
}

void reddownload_datasource_wrapper_log_set_back(void *func, void *arg) {
  RedLogSetCallback((void (*)(void *, int, const char *))func, arg);
}

void reddownload_datasource_wrapper_opt_reset(DownLoadOptWrapper *opt) {
  if (!opt) {
    return;
  }
  opt->PreDownLoadSize = -1;
  opt->cache_file_dir = nullptr;
  opt->cache_max_dir_capacity = -1;
  opt->cache_max_entries = 0;
  opt->threadpool_size = 0;
  opt->iptype = -1;
  opt->dns_cache_timeout = 0;
  opt->headers = nullptr;
  opt->useragent = nullptr;
  opt->http_proxy = nullptr;
  opt->referer = nullptr;
  opt->dnsserverip = nullptr;
  opt->max_retry = -1;
  opt->low_speed_limit = -1;
  opt->low_speed_time = -1;
  opt->connect_timeout = -1;
  opt->readasync = 1;
  opt->loadfile = 1;
  opt->tcp_buffer = -1;
  opt->dns_timeout = 0;
  opt->url_list_separator = nullptr;
  opt->DownLoadType = 0;
  opt->use_https = 0;
}

void reddownload_datasource_wrapper_init(DownLoadOptWrapper *opt) {
  DownLoadOpt *mop = new DownLoadOpt();
  if (opt->cache_file_dir) {
    mop->cache_file_dir = opt->cache_file_dir;
  }
  if (opt->cache_max_dir_capacity > 0) {
    mop->cache_max_dir_capacity = opt->cache_max_dir_capacity;
  }
  if (opt->cache_max_entries > 0) {
    mop->cache_max_entries = opt->cache_max_entries;
  }
  if (opt->threadpool_size > 0) {
    mop->threadpoolsize = opt->threadpool_size;
  }
  if (opt->dnsserverip) {
    mop->dnsserverip = opt->dnsserverip;
  }

  RedDownloadCacheManager::getinstance()->Global_init(mop);
}

void wrapper_to_downloadopt(DownLoadOpt *mop, DownLoadOptWrapper *opt) {
  if (!mop || !opt) {
    return;
  }
  if (opt->DownLoadType > 0) {
    mop->DownLoadType = opt->DownLoadType;
  }
  if (opt->PreDownLoadSize > 0) {
    mop->PreDownLoadSize = opt->PreDownLoadSize;
  }
  if (opt->cache_file_dir) {
    mop->cache_file_dir = opt->cache_file_dir;
  }
  if (opt->dnsserverip) {
    mop->dnsserverip = opt->dnsserverip;
  }
  if (opt->cache_max_dir_capacity > 0) {
    mop->cache_max_dir_capacity = opt->cache_max_dir_capacity;
  }
  if (opt->cache_max_entries > 0) {
    mop->cache_max_entries = opt->cache_max_entries;
  }
  if (opt->threadpool_size > 0) {
    mop->threadpoolsize = opt->threadpool_size;
  }
  if (opt->iptype >= 0) {
    mop->iptype = opt->iptype;
  }
  if (opt->dns_cache_timeout > 0) {
    mop->dns_cache_timeout = opt->dns_cache_timeout;
  }
  if (opt->headers) {
    string headers = opt->headers;
    mop->headers.push_back(headers);
  }
  if (opt->useragent) {
    mop->useragent = opt->useragent;
  }
  if (opt->http_proxy) {
    mop->http_proxy = opt->http_proxy;
  }
  if (opt->referer) {
    mop->referer = opt->referer;
  }
  if (opt->max_retry > 0) {
    mop->max_retry = opt->max_retry;
  }
  if (opt->low_speed_limit >= 0) {
    mop->low_speed_limit = opt->low_speed_limit;
  }
  if (opt->low_speed_time > 0) {
    mop->low_speed_time = opt->low_speed_time;
  }
  if (opt->connect_timeout > 0) {
    mop->connect_timeout = opt->connect_timeout;
  }
  mop->readasync = opt->readasync;
  mop->loadfile = opt->loadfile;
  mop->tcp_buffer = opt->tcp_buffer;
  mop->dns_timeout = opt->dns_timeout;
  if (opt->url_list_separator)
    mop->url_list_separator = opt->url_list_separator;
  mop->use_https = opt->use_https;
}

void reddownload_datasource_wrapper_dnscb(DownLoadCbWrapper *cb) {
  DownLoadCbImp *mcb = nullptr;
  if (cb) {
    mcb = new DownLoadCbImp(cb);
  }
  REDDnsCache::getinstance()->SetDnsCb(mcb);
  // AV_LOGW(LOG_TAG, "%s\n",__FUNCTION__);
}

void reddownload_datasource_wrapper_net_changed() {
  REDDnsCache::getinstance()->NetWorkChanged();
  // AV_LOGW(LOG_TAG, "%s\n",__FUNCTION__);
}

int64_t reddownload_datasource_wrapper_open(const char *url,
                                            DownLoadCbWrapper *cb,
                                            DownLoadOptWrapper *opt) {
  if ((url == nullptr) || (strlen(url) == 0))
    return -1;
  DownLoadCbImp *mcb = nullptr;
  if (cb) {
    mcb = new DownLoadCbImp(cb);
  }
  DownLoadOpt *mop = new DownLoadOpt();
  wrapper_to_downloadopt(mop, opt);
  int64_t res = RedDownloadCacheManager::getinstance()->Open(url, mcb, mop);
  return res;
}

int reddownload_datasource_wrapper_read(const char *url, int64_t uid,
                                        unsigned char *buf, int size) {
  if ((url == nullptr) || (strlen(url) == 0))
    return ERROR(EIO);
  int res = RedDownloadCacheManager::getinstance()->Read(url, uid, buf, size);
  return res;
}

int64_t reddownload_datasource_wrapper_seek(const char *url, int64_t uid,
                                            int64_t pos, int whence) {
  if ((url == nullptr) || (strlen(url) == 0))
    return -1;
  int64_t res =
      RedDownloadCacheManager::getinstance()->Seek(url, uid, pos, whence);
  return res;
}

void reddownload_datasource_wrapper_stop(const char *url, int64_t uid) {
  string cacheurl;
  if (url == nullptr) {
    RedDownloadCacheManager::getinstance()->Stop(cacheurl);
  } else {
    RedDownloadCacheManager::getinstance()->Stop(url);
  }
}

int reddownload_datasource_wrapper_close(const char *url, int64_t uid) {
  if ((url == nullptr) || (strlen(url) == 0))
    return 0;
  RedDownloadCacheManager::getinstance()->Close(url, uid);
  return 0;
}

int64_t reddownload_datasource_wrapper_cache_size(const char *url) {
  if ((url == nullptr) || (strlen(url) == 0))
    return 0;
  return RedDownloadCacheManager::getinstance()->GetCacheSize(url);
}

int64_t reddownload_datasource_wrapper_cache_size_by_uri(const char *path,
                                                         const char *uri,
                                                         int is_full_url) {
  if ((path == nullptr) || (uri == nullptr) || (strlen(uri) == 0))
    return 0;
  return RedDownloadCacheManager::getinstance()->GetCacheSizeByUri(path, uri,
                                                                   is_full_url);
}

void reddownload_global_config_set(const char *key, int value) {
  RedDownloadConfig::getinstance()->set_config(key, value);
}

int reddownload_get_url_md5(const char *url, char *outMd5Path, int Md5Length) {
  if ((url == nullptr) || (strlen(url) == 0))
    return -1;
  string strurlmd5 = RedDownloadCacheManager::getinstance()->GetUrlMd5Path(url);
  if (strurlmd5.empty()) {
    return -1;
  }
  memset(outMd5Path, 0, Md5Length);
  memcpy(outMd5Path, strurlmd5.c_str(),
         strurlmd5.size() > Md5Length ? Md5Length : strurlmd5.size());
  return 0;
}

int reddownload_get_cache_file_path(const char *path, const char *url,
                                    char **file_path) {
  if ((path == nullptr) || (url == nullptr) || (file_path == nullptr)) {
    return -1;
  }
  std::string cache_file_path =
      RedDownloadCacheManager::getinstance()->GetCacheFilePath(path, url);
  *file_path = strdup(cache_file_path.c_str());
  return 0;
}

int reddownload_get_key(const char *url, char **file_key) {
  if (url == nullptr || file_key == nullptr) {
    return -1;
  }
  std::string key = RedDownloadCacheManager::getinstance()->GetUrlMd5Path(url);
  *file_key = strdup(key.c_str());
  return 0;
}

void reddownload_get_network_quality(int *level, int *speed) {
  auto info = NetworkQuality::getInstance()->getNetInfo();
  *level = static_cast<int>(info->level);
  *speed = info->downloadSpeed;
}

void reddownload_set_download_cdn_wrapper(const char *url, int download_cdn) {
  return RedDownloadCacheManager::getinstance()->setDownloadCdn(url,
                                                                download_cdn);
}

void reddownload_get_all_cached_file(const char *dirpath, char ***cached_file,
                                     int *cached_file_len) {
  if ((dirpath == nullptr) || (cached_file == nullptr) ||
      (cached_file_len == nullptr)) {
    return;
  }
  return RedDownloadCacheManager::getinstance()->getAllCachedFile(
      dirpath, cached_file, cached_file_len);
}

int reddownload_delete_cache(const char *dirpath, const char *uri,
                             int is_full_url) {
  if ((dirpath == nullptr) || (uri == nullptr) || (strlen(uri) == 0))
    return -1;
  RedDownloadCacheManager::getinstance()->deleteCache(dirpath, uri,
                                                      is_full_url);
  return 0;
}
