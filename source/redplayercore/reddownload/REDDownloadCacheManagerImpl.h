#pragma once

#include "REDDownloadCache.h"
#include "REDDownloadCacheManager.h"

class RedDownloadCacheManagerImpl : public RedDownloadCacheManager {
public:
  RedDownloadCacheManagerImpl();
  ~RedDownloadCacheManagerImpl();

public:
  void Global_init(DownLoadOpt *opt);
  int64_t Open(const std::string &url, DownLoadListen *callback,
               DownLoadOpt *opt);
  int Read(const std::string &url, int64_t uid, uint8_t *buf, size_t nbyte);
  int64_t Seek(const std::string &url, int64_t uid, int64_t offset, int whence);
  int64_t GetCacheSize(const std::string &url);
  int64_t GetCacheSizeByUri(const std::string &path, const std::string &uri,
                            bool is_full_url);
  std::string GetCacheFilePath(const std::string &path, const std::string &url);
  // bool setOpt(const std::string &url, const string &key, const string
  // &value);
  void Stop(const std::string &url, int64_t uid = 0);
  void Close(const std::string &url, int64_t uid);
  std::string GetUrlMd5Path(const std::string &url);
  void setDownloadCdn(const char *url, int download_cdn);

  void getAllCachedFile(const std::string &dirpath, char ***cached_file,
                        int *cached_file_len);
  void deleteCache(const std::string &dirpath, const std::string &uri,
                   bool is_full_url);

private:
  shared_ptr<RedDownloadCache> addcache(const std::string &url,
                                        DownLoadOpt *opt);
  shared_ptr<RedDownloadCache> getcache(const std::string &url, int64_t uid);
  shared_ptr<RedDownloadCache> movcache(const std::string &url, int64_t uid);
  vector<std::shared_ptr<RedDownloadCache>> mveccache;
  vector<std::shared_ptr<RedDownloadCache>> mvecprecache;
  vector<std::shared_ptr<RedDownloadCache>> mvecadscache;
  std::deque<std::string> mdequeblacklist;
  std::mutex m_mutex;
  std::mutex m_file_mutex;
  std::mutex m_queue_mutex;
  bool minited;
};
