#pragma once
#include "REDDownloadListen.h"
#include "RedDownloadConfig.h"

#define REDDOWNLOAD_VERSION "V1.0"

class RedDownloadCacheManager {
public:
  static RedDownloadCacheManager *getinstance();
  static std::once_flag RedDownloadCacheManagerOnceFlag;
  static RedDownloadCacheManager *RedDownloadCacheManagerInstance;
  virtual ~RedDownloadCacheManager() = 0;

public:
  virtual void Global_init(DownLoadOpt *opt) = 0;
  virtual int64_t Open(const std::string &url, DownLoadListen *callback,
                       DownLoadOpt *opt) = 0;
  virtual int Read(const std::string &url, int64_t uid, uint8_t *buf,
                   size_t nbyte) = 0;
  virtual int64_t Seek(const std::string &url, int64_t uid, int64_t offset,
                       int whence) = 0;
  virtual int64_t GetCacheSize(const std::string &url) = 0;
  virtual int64_t GetCacheSizeByUri(const std::string &path,
                                    const std::string &uri,
                                    bool is_full_url) = 0;
  virtual std::string GetCacheFilePath(const std::string &path,
                                       const std::string &url) = 0;
  // bool setOpt(const std::string &url, const string &key, const string
  // &value);
  virtual void Stop(const std::string &url, int64_t uid = 0) = 0;
  virtual void Close(const std::string &url, int64_t uid) = 0;
  virtual std::string GetUrlMd5Path(const std::string &url) = 0;
  virtual void setDownloadCdn(const char *url, int download_cdn) = 0;
  virtual void getAllCachedFile(const std::string &dirpath, char ***cached_file,
                                int *cached_file_len) = 0;
  virtual void deleteCache(const std::string &dirpath, const std::string &uri,
                           bool is_full_url) = 0;
};
