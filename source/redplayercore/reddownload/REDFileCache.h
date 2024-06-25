#pragma once

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#define DOWNLOAD_SHARD_SIZE (1024 * 1024)
#define MAX_CACHE_ENTRIES 10
#define CONFIG_MAX_LINE 1024
#define CACHE_MAX_DIR_CAPACITY (60 * 1024 * 1024)
using namespace std;

struct REDCacheInfo {
  std::uint32_t data_amount;
  std::uint64_t physical_pos;
  std::uint64_t logical_pos;
  REDCacheInfo() : data_amount(0), physical_pos(0), logical_pos(0) {}
  REDCacheInfo(std::uint64_t start_pos_, std::uint32_t data_amount_)
      : data_amount(data_amount_), physical_pos(0), logical_pos(start_pos_) {}
  REDCacheInfo(std::uint64_t start_pos_, std::uint32_t data_amount_,
               std::uint64_t physical_pos_)
      : data_amount(data_amount_), physical_pos(physical_pos_),
        logical_pos(start_pos_) {}
};

struct REDCachePath {
  std::string key_url;
  std::string value_cache_path;
  REDCachePath *next;
  REDCachePath *prev;
  bool mpin; // file is in use
  FILE *mfd;
  FILE *mfd_map;
  int64_t mfilesize;
  int64_t mcachesize;
  int mperiodsize;
  std::unordered_map<std::uint64_t, REDCacheInfo *> cache_info_map;
  REDCachePath()
      : next(nullptr), prev(nullptr), mpin(false), mfd(nullptr),
        mfd_map(nullptr), mfilesize(-1), mcachesize(0), mperiodsize(0) {}
  REDCachePath(const std::string &url, const std::string &path)
      : key_url(url), value_cache_path(path), next(nullptr), prev(nullptr),
        mpin(false), mfd(nullptr), mfd_map(nullptr), mfilesize(-1),
        mcachesize(0), mperiodsize(0) {}
  REDCachePath(const std::string &url, const std::string &path, FILE *fd)
      : key_url(url), value_cache_path(path), next(nullptr), prev(nullptr),
        mpin(false), mfd(fd), mfd_map(nullptr), mfilesize(-1), mcachesize(0),
        mperiodsize(0) {}
};

class REDFileCache {
public:
  REDFileCache(int max_cache_entries_ = MAX_CACHE_ENTRIES,
               int64_t max_dir_capacity_ = CACHE_MAX_DIR_CAPACITY);
  ~REDFileCache();

  /*load the local cache file to cachepathmap and cacheinfomap, when the app
   * start*/
  int GetDirectoryFiles();
  int set_cache_path(const std::string &path);
  void set_max_cache_entries(std::uint32_t max_entries);
  void set_max_dir_capacity(std::int64_t max_capacity);
  void set_download_cache_size(int downloadcachesize);

  /*update the download range info of the file(video), when loadtofile*/
  int update_cache_info(const std::string &uri, std::uint8_t *data,
                        std::int64_t start_pos, std::uint32_t length);
  /*return the fd, througth the request of the offset*/
  int get_cache_file(const std::string &uri, std::uint8_t *data,
                     std::uint64_t offset, int bufsize);
  /*set or get total file size*/
  void set_file_size(const std::string &uri, int64_t filesize);
  int64_t get_file_size(const std::string &uri, int &rangesize);
  /*get the file cache size*/
  int64_t get_cache_size(const std::string &uri);
  void close_cache_file(const std::string &uri);

  void get_all_cache_files(const std::string &dirpath, char ***cached_file,
                           int *cached_file_len);
  void delete_cache_file(const std::string &uri);
  void set_cache_type(int cache_type) { m_cache_type = cache_type; }

private:
  bool inited{false};
  int mdownloadcachesize{DOWNLOAD_SHARD_SIZE};
  std::int64_t physical_total_size{0};

private:
  std::string base_local_path;
  int max_cache_entries;
  int64_t max_dir_capacity;
  int m_cache_type{0};

  /*LRU for REDCachePath*/
  std::unordered_map<std::string, REDCachePath *> cache_path_map;
  REDCachePath *cache_path_head;
  REDCachePath *cache_path_tail;
  std::mutex map_mutex;
  std::mutex path_mutex;
  void cache_path_init();
  void add_to_head(REDCachePath *path);
  void move_to_head(REDCachePath *path);
  REDCachePath *delete_tail_cache();
  REDCachePath *delete_cache_by_uri(const std::string uri);

  /*find the REDCachePath with the uri*/
  REDCachePath *search_cache(const std::string &uri);
  /*add the file(video) to cachepathmap, when GetDirectoryFiles*/
  void push_cache(const std::string &uri);
  /*parse the localmapfile and load the file info to infomap, when
   * GetDirectoryFiles*/
  int parse_cache_info(REDCachePath *cache_path);
  /*save the file(video) info to map, when the file closed*/
  int save_cache_info(REDCachePath *cache_path);
  /*clear local cache after GetDirectoryFiles(), need recreate cache file*/
  int recreate_cache_file(REDCachePath *cache_path);
  /*create local cache dir, if its null*/
  int create_cache_dir(const std::string &path);
  /*calculate the size of the directory */
  int64_t get_dir_size(const std::string &path);
  /*unlink(delete) the localfile and localmapfile*/
  bool delete_local_file(const std::string &uri = "");
};
