#pragma once

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "REDFileCache.h"
using namespace std;

class REDFileManager {
public:
  static REDFileManager *getInstance();
  static void delInstance();
  ~REDFileManager();

  /*init option for filecache*/
  void set_dir_capacity(const std::string &path, std::uint32_t max_entries,
                        std::int64_t max_capacity, int downloadcachesize,
                        int cache_type = 0);

  /*update the download range info of the file(video), when loadtofile*/
  int update_cache_info(const std::string &uri, const std::string &dirpath,
                        std::uint8_t *data, std::int64_t start_pos,
                        std::uint32_t length);
  /*return the fd, througth the request of the offset*/
  int get_cache_file(const std::string &uri, const std::string &dirpath,
                     std::uint8_t *data, std::uint64_t offset, int bufsize);
  /*set or get total file size*/
  void set_file_size(const std::string &uri, const std::string &dirpath,
                     int64_t filesize);
  int64_t get_file_size(const std::string &uri, const std::string &dirpath,
                        int &rangesize);
  /*get the file cache size*/
  int64_t get_cache_size(const std::string &uri, const std::string &dirpath);
  void close_cache_file(const std::string &uri, const std::string &dirpath);

  void get_all_cache_files(const std::string &dirpath, char ***cached_file,
                           int *cached_file_len);
  void delete_cache(const std::string &dirpath, const std::string &uri);

private:
  REDFileManager();
  static REDFileManager *minstance;
  std::shared_ptr<REDFileCache> getfilecache(const std::string &dirpath);

private:
  std::unordered_map<std::string, std::shared_ptr<REDFileCache>> cachefilemap;
  std::mutex mutex;
};
