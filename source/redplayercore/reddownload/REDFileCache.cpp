#include "REDFileCache.h"

#include <inttypes.h>

#include "REDDownloadListen.h"
#include "RedDownloadConfig.h"
#include "RedLog.h"
#define LOG_TAG "RedFileCache"
// REDFileCache REDFileCache::mfilecache(MAX_CACHE_ENTRIES);

REDFileCache::REDFileCache(int max_cache_entries_, int64_t max_dir_capacity_)
    : max_cache_entries(max_cache_entries_),
      max_dir_capacity(max_dir_capacity_), map_mutex(), path_mutex() {
  cache_path_init();
  // AV_LOGI(LOG_TAG, "REDCache - %s init\n", __FUNCTION__);
}

REDFileCache::~REDFileCache() {
  std::lock_guard<std::mutex> lock(path_mutex);
  while (cache_path_head != nullptr) {
    REDCachePath *temp = cache_path_head->next;
    std::unordered_map<std::uint64_t, REDCacheInfo *>::iterator infoMapIter;
    infoMapIter = cache_path_head->cache_info_map.begin();
    while (infoMapIter != cache_path_head->cache_info_map.end()) {
      delete infoMapIter->second;
      infoMapIter->second = nullptr;
      ++infoMapIter;
    }
    cache_path_head->cache_info_map.clear();
    delete cache_path_head;
    cache_path_head = temp;
  }
  {
    std::lock_guard<std::mutex> lock(map_mutex);
    cache_path_map.clear();
  }
  AV_LOGW(LOG_TAG, "REDCache - %s destroy\n", __FUNCTION__);
}

int REDFileCache::set_cache_path(const std::string &path) {
  std::lock_guard<std::mutex> lock(map_mutex);
  std::string cache_path = path;
  DIR *dp = nullptr;
  if (inited)
    return 0;
  if (cache_path.back() != '/') {
    cache_path += '/';
  }
  if ((dp = opendir(cache_path.c_str())) == nullptr) {
    if (create_cache_dir(cache_path) != 0) {
      AV_LOGW(LOG_TAG, "REDCache - %s create dir - %s failed!\n", __FUNCTION__,
              cache_path.c_str());
      return -1;
    }
  } else {
    closedir(dp);
  }

  base_local_path = cache_path;
  AV_LOGW(LOG_TAG, "REDCache - set cache path:%s\n", base_local_path.c_str());
  return 0;
}

void REDFileCache::set_max_cache_entries(std::uint32_t max_entries) {
  std::lock_guard<std::mutex> lock(map_mutex);
  if (inited && (m_cache_type !=
                 DOWNLOADKAIPINGADS)) // todo(renwei) ads should call global
                                      // init to set max_capacity only once
    return;
  max_cache_entries = max_entries;
  // AV_LOGW(LOG_TAG, "REDCache - set max cache entries:%d\n",
  // max_cache_entries);
}

void REDFileCache::set_max_dir_capacity(std::int64_t max_capacity) {
  std::lock_guard<std::mutex> lock(map_mutex);
  if (inited && (m_cache_type != DOWNLOADKAIPINGADS))
    return;
  max_dir_capacity = max_capacity;
  // AV_LOGW(LOG_TAG, "REDCache - set max cache capacity:%" PRId64 "\n",
  // max_dir_capacity);
}

void REDFileCache::set_download_cache_size(int downloadcachesize) {
  std::lock_guard<std::mutex> lock(map_mutex);
  if (inited || downloadcachesize <= 0)
    return;
  mdownloadcachesize = downloadcachesize;
}

void REDFileCache::set_file_size(const std::string &uri, int64_t filesize) {
  std::lock_guard<std::mutex> lock(map_mutex);
  REDCachePath *rcp = search_cache(uri);
  if (rcp != nullptr) {
    rcp->mfilesize = filesize;
  } else {
    AV_LOGW(LOG_TAG, "REDCache - set file size failed\n");
  }
}

int64_t REDFileCache::get_file_size(const std::string &uri, int &rangesize) {
  std::lock_guard<std::mutex> lock(map_mutex);
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  REDCachePath *rcp = search_cache(uri);
  if (rcp != nullptr) {
    if (rcp->mperiodsize > 0)
      rangesize = rcp->mperiodsize;
    return rcp->mfilesize;
  } else {
    AV_LOGW(LOG_TAG, "REDCache - get file size failed\n");
    return -1;
  }
}

int64_t REDFileCache::get_cache_size(const std::string &uri) {
  std::lock_guard<std::mutex> lock(map_mutex);
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  if (uri.empty() || base_local_path.empty()) {
    AV_LOGW(LOG_TAG, "REDCache - %s uri or base_local_path is empty\n",
            __FUNCTION__);
    return 0;
  }
  REDCachePath *cache_path = search_cache(uri);
  if (cache_path == nullptr) {
    AV_LOGW(LOG_TAG, "REDCache - %s cache_path - %s is null\n", __FUNCTION__,
            uri.c_str());
    return 0;
  }

  return cache_path->mcachesize;
}

void REDFileCache::push_cache(const std::string &uri) {
  std::lock_guard<std::mutex> lock(map_mutex);
  std::unordered_map<std::string, REDCachePath *>::iterator mapIter;
  if ((mapIter = cache_path_map.find(uri)) != cache_path_map.end()) {
    // AV_LOGW(LOG_TAG, "REDCache - %s the file - %s exist in chache\n",
    // __FUNCTION__, uri.c_str());
    REDCachePath *cachePath = cache_path_map[uri];
    move_to_head(cachePath);
  } else {
    // AV_LOGW(LOG_TAG, "REDCache - %s the file - %s does not exist in
    // cache\n", __FUNCTION__, uri.c_str());
    const std::string local_path = base_local_path + uri;
    REDCachePath *cachePath = new REDCachePath(uri, local_path);
    if (!cachePath) {
      AV_LOGW(LOG_TAG, "REDCache - new cache path - %s is failed!\n",
              uri.c_str());
      return;
    }
    cache_path_map[uri] = cachePath;
    add_to_head(cachePath);
    if (RedDownloadConfig::getinstance()->get_config_value(FIX_GETDIR_SIZE) >
        0) {
      // AV_LOGW(LOG_TAG, "REDCache - %s physical_total_size:%" PRId64
      // "\n", __FUNCTION__, physical_total_size);
      if (cache_path_map.size() > max_cache_entries ||
          physical_total_size > max_dir_capacity) {
        delete_local_file();
      }
    } else if (cache_path_map.size() > max_cache_entries ||
               get_dir_size(base_local_path) > max_dir_capacity) {
      delete_local_file();
    }
  }
}

REDCachePath *REDFileCache::search_cache(const std::string &uri) {
  if (uri.empty()) {
    AV_LOGW(LOG_TAG, "REDCache - %s uri is nullptr!\n", __FUNCTION__);
    return nullptr;
  }

  std::unordered_map<std::string, REDCachePath *>::iterator mapIter;
  if ((mapIter = cache_path_map.find(uri)) != cache_path_map.end()) {
    REDCachePath *cachePath = cache_path_map[uri];
    return cachePath;
  }

  return nullptr;
}

int REDFileCache::GetDirectoryFiles() {
  {
    std::lock_guard<std::mutex> lock(map_mutex);
    if (inited)
      return 0;
    inited = true;
    // AV_LOGW(LOG_TAG, "REDCache - Get Directory Files Once, path %s\n",
    // base_local_path.c_str());
  }
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;
  std::vector<std::string> unlink_files;

  if ((dp = opendir(base_local_path.c_str())) == nullptr) {
    AV_LOGW(LOG_TAG, "REDCache - Cannot open dir: %s; error: %s\n",
            base_local_path.c_str(), strerror(errno));
    return -1;
  }

  while ((entry = readdir(dp)) != nullptr) {
    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0 ||
        strcmp(".DS_Store", entry->d_name) == 0) {
      continue;
    }
    if (strstr(entry->d_name, "-map") != nullptr) {
      continue;
    }

    std::string local_path = base_local_path + entry->d_name;
    lstat(local_path.c_str(), &statbuf);
    if (!S_ISDIR(statbuf.st_mode)) {
      std::string map_local_path = base_local_path + entry->d_name + "-map";
      lstat(map_local_path.c_str(), &statbuf);
      if (statbuf.st_size == 0) {
        unlink_files.push_back(local_path);
        continue;
      }
      push_cache(entry->d_name);
      REDCachePath *cache_path = search_cache(entry->d_name);
      if (cache_path != nullptr) {
        parse_cache_info(cache_path);
        physical_total_size += cache_path->mcachesize;
      }
    }
  }

  for (std::vector<std::string>::iterator it = unlink_files.begin();
       it != unlink_files.end(); ++it) {
    std::string local_path = *it;
    std::string map_local_path = local_path + "-map";
    unlink(local_path.c_str());
    unlink(map_local_path.c_str());
  }
  if (RedDownloadConfig::getinstance()->get_config_value(FIX_GETDIR_SIZE) > 0) {
    while (physical_total_size > max_dir_capacity) {
      AV_LOGW(LOG_TAG, "REDCache - %s max_dir_capacity:%" PRId64 "\n",
              __FUNCTION__, max_dir_capacity);
      if (!delete_local_file()) {
        break;
      }
    }
  } else {
    while (get_dir_size(base_local_path) > max_dir_capacity) {
      AV_LOGW(LOG_TAG, "REDCache - %s max_dir_capacity:%" PRId64 "\n",
              __FUNCTION__, max_dir_capacity);
      if (!delete_local_file()) {
        break;
      }
    }
  }
  closedir(dp);
  AV_LOGW(LOG_TAG, "REDCache - Get Directory Files Once done, path %s\n",
          base_local_path.c_str());
  return 0;
}

int REDFileCache::parse_cache_info(REDCachePath *cache_path) {
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  char string_line[CONFIG_MAX_LINE] = {0};
  char *ptr = nullptr;
  uint64_t str_len = 0;
  uint64_t entry_logical_pos = 0;
  uint64_t entry_physical_pos = 0;
  uint64_t entry_data_amount = 0;

  const char *dst_file_size = "total_file_size:";
  const char *dst_cache_size = "cache_file_size:";
  const char *dst_period_size = "cache_period_size:";
  const char *dst_entry_logical = "entry_logical_pos:";
  const char *dst_entry_data = "entry_data_amount:";
  const char *dst_entry_physical = "entry_physical_pos:";

  std::string map_file_path = base_local_path + cache_path->key_url + "-map";
  FILE *fp = fopen(map_file_path.c_str(), "r+");
  if (!fp) {
    AV_LOGW(LOG_TAG, "REDCache - parse cache info open map file failed!\n");
    return -1;
  }

  while (!feof(fp)) {
    memset(string_line, 0, CONFIG_MAX_LINE);
    fgets(string_line, CONFIG_MAX_LINE, fp);

    if ((ptr = strstr(string_line, dst_file_size)) != nullptr) {
      ptr += strlen(dst_file_size);
      str_len = strlen(ptr);
      for (int i = 0; i < str_len; i++) {
        if ((ptr)[i] < '0' || (ptr)[i] > '9') {
          (ptr)[i] = '\0';
          break;
        }
      }
      cache_path->mfilesize = static_cast<int>(strtol(ptr, NULL, 10));
    } else if ((ptr = strstr(string_line, dst_cache_size)) != nullptr) {
      ptr += strlen(dst_cache_size);
      str_len = strlen(ptr);
      for (int i = 0; i < str_len; i++) {
        if ((ptr)[i] < '0' || (ptr)[i] > '9') {
          (ptr)[i] = '\0';
          break;
        }
      }
      cache_path->mcachesize = static_cast<int>(strtol(ptr, NULL, 10));
    } else if ((ptr = strstr(string_line, dst_period_size)) != nullptr) {
      ptr += strlen(dst_period_size);
      str_len = strlen(ptr);
      for (int i = 0; i < str_len; i++) {
        if ((ptr)[i] < '0' || (ptr)[i] > '9') {
          (ptr)[i] = '\0';
          break;
        }
      }
      cache_path->mperiodsize = static_cast<int>(strtol(ptr, NULL, 10));
    } else if ((ptr = strstr(string_line, dst_entry_logical)) != nullptr) {
      ptr += strlen(dst_entry_logical);
      str_len = strlen(ptr);
      for (int i = 0; i < str_len; i++) {
        if ((ptr)[i] < '0' || (ptr)[i] > '9') {
          (ptr)[i] = '\0';
          break;
        }
      }
      entry_logical_pos = static_cast<int>(strtol(ptr, NULL, 10));
    } else if ((ptr = strstr(string_line, dst_entry_data)) != nullptr) {
      ptr += strlen(dst_entry_data);
      str_len = strlen(ptr);
      for (int i = 0; i < str_len; i++) {
        if ((ptr)[i] < '0' || (ptr)[i] > '9') {
          (ptr)[i] = '\0';
          break;
        }
      }
      entry_data_amount = static_cast<int>(strtoll(ptr, NULL, 10));
    } else if ((ptr = strstr(string_line, dst_entry_physical)) != nullptr) {
      ptr += strlen(dst_entry_physical);
      str_len = strlen(ptr);
      for (int i = 0; i < str_len; i++) {
        if ((ptr)[i] < '0' || (ptr)[i] > '9') {
          (ptr)[i] = '\0';
          break;
        }
      }
      entry_physical_pos = static_cast<int>(strtoll(ptr, NULL, 10));
    } else if (strstr(string_line, "entry_info_flush") != nullptr) {
      REDCacheInfo *cacheInfo = new REDCacheInfo(
          entry_logical_pos, static_cast<uint32_t>(entry_data_amount),
          entry_physical_pos);
      if (cacheInfo == nullptr) {
        AV_LOGW(LOG_TAG, "REDCache - %s new cache info failed\n", __FUNCTION__);
        fclose(fp);
        fp = nullptr;
        return -1;
      }
      cache_path->cache_info_map[entry_logical_pos] = cacheInfo;
    }
  }
  if (cache_path->mperiodsize == 0) {
    cache_path->mperiodsize = DOWNLOAD_SHARD_SIZE;
  }
  fclose(fp);
  fp = nullptr;

  return 0;
}

int REDFileCache::save_cache_info(REDCachePath *cache_path) {
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  std::string map_file_path = base_local_path + cache_path->key_url + "-map";
  cache_path->mfd_map = fopen(map_file_path.c_str(), "r+");

  if (cache_path->mfd_map == nullptr) {
    AV_LOGW(LOG_TAG, "REDCache - %s fopen map file - %s is failed!\n",
            __FUNCTION__, map_file_path.c_str());
    return -1;
  }
  std::string config("");
  config += ("total_file_size:" + std::to_string(cache_path->mfilesize) + '\n');
  config +=
      ("cache_file_size:" + std::to_string(cache_path->mcachesize) + '\n');
  if (cache_path->mperiodsize == 0) {
    cache_path->mperiodsize = mdownloadcachesize;
  }
  config +=
      ("cache_period_size:" + std::to_string(cache_path->mperiodsize) + '\n');
  std::unordered_map<std::uint64_t, REDCacheInfo *>::iterator infoMapIter;
  infoMapIter = cache_path->cache_info_map.begin();
  while (infoMapIter != cache_path->cache_info_map.end()) {
    config += ("entry_logical_pos:" +
               std::to_string(infoMapIter->second->logical_pos) + '\n');
    config += ("entry_data_amount:" +
               std::to_string(infoMapIter->second->data_amount) + '\n');
    config += ("entry_physical_pos:" +
               std::to_string(infoMapIter->second->physical_pos) + '\n');
    config += "entry_info_flush\n";
    ++infoMapIter;
  }
  size_t size = fwrite(config.c_str(), 1, config.length(), cache_path->mfd_map);
  if (size != config.length()) {
    AV_LOGW(LOG_TAG, "REDCache - %s fwrite(%zu) error(%s)!\n", __FUNCTION__,
            size, strerror(errno));
    fclose(cache_path->mfd_map);
    remove(map_file_path.c_str());
    return -1;
  }
  fclose(cache_path->mfd_map);
  cache_path->mfd_map = nullptr;
  return 0;
}

int REDFileCache::recreate_cache_file(REDCachePath *cache_path) {
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;

  if ((dp = opendir(base_local_path.c_str())) == nullptr) {
    AV_LOGW(LOG_TAG, "REDCache - Cannot open dir: %s; error: %s\n",
            base_local_path.c_str(), strerror(errno));
    return -1;
  }

  while ((entry = readdir(dp)) != nullptr) {
    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0 ||
        strcmp(".DS_Store", entry->d_name) == 0) {
      continue;
    }
    if (strstr(entry->d_name, "-map") != nullptr) {
      continue;
    }

    std::string local_path = base_local_path + entry->d_name;
    lstat(local_path.c_str(), &statbuf);

    if (!S_ISDIR(statbuf.st_mode)) {
      if (strcmp(cache_path->key_url.c_str(), entry->d_name) == 0) {
        cache_path->mfd = fopen(cache_path->value_cache_path.c_str(), "r+");
        if (cache_path->mfd == nullptr) {
          AV_LOGW(LOG_TAG, "REDCache - %s open file - %s failed!\n",
                  __FUNCTION__, cache_path->value_cache_path.c_str());
          closedir(dp);
          return -1;
        }
        closedir(dp);
        return 0;
      }
    }
  }

  std::unordered_map<std::uint64_t, REDCacheInfo *>::iterator infoMapIter;
  infoMapIter = cache_path->cache_info_map.begin();
  while (infoMapIter != cache_path->cache_info_map.end()) {
    delete infoMapIter->second;
    infoMapIter->second = nullptr;
    ++infoMapIter;
  }
  cache_path->cache_info_map.clear();

  std::string local_map_path = cache_path->value_cache_path + "-map";
  cache_path->mfd_map = fopen(local_map_path.c_str(), "w+");
  if (cache_path->mfd_map == nullptr) {
    AV_LOGW("REDCache - %s recreate file mfd_map failed!\n", __FUNCTION__);
    closedir(dp);
    return -1;
  }
  fclose(cache_path->mfd_map);
  cache_path->mfd_map = nullptr;

  cache_path->mfd = fopen(cache_path->value_cache_path.c_str(), "w+");
  if (cache_path->mfd == nullptr) {
    AV_LOGW("REDCache - %s recreate file mfd failed!\n", __FUNCTION__);
    closedir(dp);
    return -1;
  }
  closedir(dp);

  return 0;
}

int REDFileCache::create_cache_dir(const std::string &path) {
  std::string dir = path;
  for (int i = 1; i < dir.length(); ++i) {
    if (dir[i] == '/') {
      dir[i] = 0;
      if (access(dir.c_str(), F_OK) != 0) {
        if (mkdir(dir.c_str(), 0755) == -1) {
          AV_LOGW(LOG_TAG, "REDCache - %s mkdir - %s failed!\n", __FUNCTION__,
                  dir.c_str());
          return -1;
        }
      }
      dir[i] = '/';
    }
  }

  return 0;
}

int64_t REDFileCache::get_dir_size(const std::string &path) {
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;
  int64_t total_size = 0;
  std::string local_path = path;

  if (local_path.back() != '/') {
    local_path += '/';
  }
  if ((dp = opendir(local_path.c_str())) == nullptr) {
    AV_LOGW(LOG_TAG, "REDCache - %s Cannot open dir: %s; error: %s\n",
            __FUNCTION__, local_path.c_str(), strerror(errno));
    return -1;
  }

  while ((entry = readdir(dp)) != NULL) {
    if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0 ||
        strcmp(".DS_Store", entry->d_name) == 0) {
      continue;
    }

    std::string file_path = local_path + entry->d_name;
    lstat(file_path.c_str(), &statbuf);
    // if (S_ISDIR(statbuf.st_mode)) {
    //     int64_t sub_dir_size = get_dir_size(file_path);
    //     total_size += sub_dir_size;
    // } else {
    //     total_size += statbuf.st_size;
    // }
    if (!S_ISDIR(statbuf.st_mode)) {
      total_size += statbuf.st_size;
    }
  }

  closedir(dp);
  return total_size;
}

bool REDFileCache::delete_local_file(const std::string &uri) {
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  REDCachePath *tailCachePath = nullptr;
  if (uri == "") {
    tailCachePath = delete_tail_cache();
  } else {
    tailCachePath = delete_cache_by_uri(uri);
  }

  if (tailCachePath != nullptr && !tailCachePath->mpin) {
    AV_LOGW(LOG_TAG,
            "REDCache - %s delete tail cache - %s, dirsize %" PRId64
            ", cachesize %" PRId64 "\n",
            __FUNCTION__, tailCachePath->key_url.c_str(), physical_total_size,
            tailCachePath->mcachesize);
    if (tailCachePath->mcachesize > 0) {
      physical_total_size -= tailCachePath->mcachesize;
    }
    std::unordered_map<std::uint64_t, REDCacheInfo *>::iterator infoMapIter;
    infoMapIter = tailCachePath->cache_info_map.begin();
    while (infoMapIter != tailCachePath->cache_info_map.end()) {
      delete infoMapIter->second;
      ++infoMapIter;
    }
    tailCachePath->cache_info_map.clear();
    cache_path_map.erase(tailCachePath->key_url);
    std::string map_local_path = tailCachePath->value_cache_path + "-map";
    unlink(tailCachePath->value_cache_path.c_str());
    unlink(map_local_path.c_str());
    delete tailCachePath;
    return true;
  }
  return false;
}

int REDFileCache::update_cache_info(const std::string &uri, std::uint8_t *data,
                                    std::int64_t start_pos,
                                    std::uint32_t length) {
  std::lock_guard<std::mutex> lock(map_mutex);
  // AV_LOGW(LOG_TAG, "REDCache -- %s\n", __FUNCTION__);
  if (uri.empty()) {
    AV_LOGW(LOG_TAG, "REDCache - %s uri is empty!\n", __FUNCTION__);
    return -1;
  }
  std::unordered_map<std::string, REDCachePath *>::iterator mapIter;
  if ((mapIter = cache_path_map.find(uri)) != cache_path_map.end()) {
    REDCachePath *cachePath = cache_path_map[uri];
    if (!cachePath) {
      AV_LOGW(LOG_TAG,
              "REDCache - %s the key_url - %s ,value cachePath is empty!\n",
              __FUNCTION__, uri.c_str());
      return -1;
    }
    if (cachePath->mfd == nullptr) {
      cachePath->mfd = fopen(cachePath->value_cache_path.c_str(), "r+");
      if (cachePath->mfd == nullptr) {
        AV_LOGW(LOG_TAG,
                "REDCache - %s mfd is null and reopen is failed! "
                "return !\n",
                __FUNCTION__);
        return -1;
      }
      AV_LOGW(LOG_TAG, "REDCache - %s mfd is null and reopen !\n",
              __FUNCTION__);
      cachePath->mpin = true;
    }
    std::unordered_map<std::uint64_t, REDCacheInfo *>::iterator infoMapIter;
    if ((infoMapIter = cachePath->cache_info_map.find(start_pos)) !=
        cachePath->cache_info_map.end()) {
      REDCacheInfo *cacheInfo = infoMapIter->second;
      if (cacheInfo->data_amount < length) {
        if (cachePath->mfd == nullptr) {
          AV_LOGW(LOG_TAG, "REDCache - %s mfd is null, return !\n",
                  __FUNCTION__);
          return -1;
        }
        if (fseek(cachePath->mfd, static_cast<int64_t>(cacheInfo->physical_pos),
                  SEEK_SET) != 0) {
          AV_LOGW(LOG_TAG, "REDCache - %s fseek error(%s)!\n", __FUNCTION__,
                  strerror(errno));
          return -1;
        }
        if (cachePath->mperiodsize > 0) {
          size_t size;
          if ((size = fwrite(data, 1, cachePath->mperiodsize,
                             cachePath->mfd)) != cachePath->mperiodsize) {
            AV_LOGW(LOG_TAG, "REDCache - %s fwrite(%zu) error(%s)!\n",
                    __FUNCTION__, size, strerror(errno));
            delete cacheInfo;
            cachePath->cache_info_map.erase(infoMapIter);
            return -1;
          }
        } else {
          size_t size;
          if ((size = fwrite(data, 1, mdownloadcachesize, cachePath->mfd)) !=
              mdownloadcachesize) {
            AV_LOGW(LOG_TAG, "REDCache - %s fwrite(%zu) error(%s)!\n",
                    __FUNCTION__, size, strerror(errno));
            delete cacheInfo;
            cachePath->cache_info_map.erase(infoMapIter);
            return -1;
          }
        }
        cachePath->mcachesize =
            cachePath->mcachesize - cacheInfo->data_amount + length;
        physical_total_size += length - cacheInfo->data_amount;
        cacheInfo->data_amount = length;
        // AV_LOGI(LOG_TAG, "REDCache - %s(fix) pos:%lld, len:%lld\n",
        // __FUNCTION__, cacheInfo->physical_pos, length);
      } else {
        // AV_LOGI(LOG_TAG, "REDCache - %s(fix) data_amount %lld >= len
        // %lld\n", __FUNCTION__, cacheInfo->data_amount, length);
      }
      return 0;
    } else {
      if ((infoMapIter = cachePath->cache_info_map.find(start_pos)) !=
          cachePath->cache_info_map.end()) {
        REDCacheInfo *cacheInfo = cachePath->cache_info_map[start_pos];
        if (cacheInfo->data_amount < length) {
          cachePath->mcachesize =
              cachePath->mcachesize - cacheInfo->data_amount + length;
          physical_total_size += length - cacheInfo->data_amount;
          cacheInfo->data_amount = length;
          if (cachePath->mfd == nullptr) {
            AV_LOGW(LOG_TAG, "REDCache - %s mfd is null, return !\n",
                    __FUNCTION__);
            return -1;
          }
          fseek(cachePath->mfd, static_cast<int64_t>(cacheInfo->physical_pos),
                SEEK_SET);
          // int ret = 0;
          if (cachePath->mperiodsize > 0) {
            fwrite(data, 1, cachePath->mperiodsize, cachePath->mfd);
          } else {
            fwrite(data, 1, mdownloadcachesize, cachePath->mfd);
          }
          AV_LOGW(LOG_TAG, "REDCache - %s pos:%" PRIu64 ", len:%" PRIu32 "\n",
                  __FUNCTION__, cacheInfo->physical_pos, length);
        } else {
          AV_LOGW(LOG_TAG,
                  "REDCache - %s data_amount %" PRIu32 " >= len %" PRIu32 "\n",
                  __FUNCTION__, cacheInfo->data_amount, length);
        }
        return 0;
      } else {
        REDCacheInfo *cacheInfo = new REDCacheInfo(start_pos, length);
        if (cacheInfo != nullptr) {
          cachePath->cache_info_map[start_pos] = cacheInfo;
          cachePath->mcachesize += length;
          physical_total_size += length;
          // cacheInfo->logical_pos = start_pos;
          std::uint64_t fd_pos = 0;

          if (cachePath->mfd == nullptr) {
            AV_LOGW(LOG_TAG,
                    "REDCache - %s mfd null with start pos ,return !\n",
                    __FUNCTION__);
            return -1;
          }
          fseek(cachePath->mfd, 0, SEEK_END);
          fd_pos = ftell(cachePath->mfd);
          if (cachePath->mperiodsize > 0) {
            cacheInfo->physical_pos =
                (fd_pos / cachePath->mperiodsize) * cachePath->mperiodsize;
            fwrite(data, 1, cachePath->mperiodsize, cachePath->mfd);
          } else {
            cacheInfo->physical_pos =
                (fd_pos / mdownloadcachesize) * mdownloadcachesize;
            fwrite(data, 1, mdownloadcachesize, cachePath->mfd);
          }
          // int ret = 0;
          AV_LOGW(LOG_TAG,
                  "REDCache - %s new pos:%" PRIu64 ", len:%" PRIu32
                  ", fd_pos:%" PRIu64 "\n",
                  __FUNCTION__, cacheInfo->physical_pos, length, fd_pos);
          return 0;
        }
        AV_LOGW(LOG_TAG,
                "REDCache - %s new cache info with the key_range - %" PRId64
                "failed!\n",
                __FUNCTION__, start_pos);
      }
    }
  }

  return -1;
}

int REDFileCache::get_cache_file(const std::string &uri, std::uint8_t *data,
                                 std::uint64_t offset, int bufsize) {
  std::lock_guard<std::mutex> lock(map_mutex);
  uint32_t amount_data = 0;
  std::unordered_map<std::string, REDCachePath *>::iterator mapIter;
  REDCachePath *cachePath = nullptr;
  if ((mapIter = cache_path_map.find(uri)) != cache_path_map.end()) {
    cachePath = cache_path_map[uri];
    // AV_LOGW(LOG_TAG, "REDCache - %s cachePath->mpin is:%d\n",
    // __FUNCTION__, cachePath->mpin);
    cachePath->mpin = true;
    move_to_head(cachePath);
    std::uint64_t key_range;
    if (cachePath->mperiodsize > 0) {
      key_range = (offset / cachePath->mperiodsize) * cachePath->mperiodsize;
    } else {
      key_range = (offset / mdownloadcachesize) * mdownloadcachesize;
    }
    if (cachePath->mfd == nullptr) {
      cachePath->mfd = fopen(cachePath->value_cache_path.c_str(), "r+");
      if (cachePath->mfd == nullptr) {
        int ret = recreate_cache_file(cachePath);
        if (ret != 0) {
          AV_LOGW(LOG_TAG, "REDCache - %s recreate cache file failed!\n",
                  __FUNCTION__);
          return -1;
        }
      }
    }

    std::unordered_map<std::uint64_t, REDCacheInfo *>::iterator infoMapIter;
    if ((infoMapIter = cachePath->cache_info_map.find(key_range)) !=
        cachePath->cache_info_map.end()) {
      amount_data = cachePath->cache_info_map[key_range]->data_amount;
      std::uint64_t seek_pos =
          cachePath->cache_info_map[key_range]->physical_pos;
      int ret = fseek(cachePath->mfd, static_cast<int64_t>(seek_pos), SEEK_SET);
      if (ret != 0) {
        AV_LOGW(LOG_TAG,
                "REDCache - %s fseek to seek_pos:%" PRIu64 "is failed!\n",
                __FUNCTION__, seek_pos);
        return -1;
      }
    } else {
      int ret = fseek(cachePath->mfd, 0, SEEK_END);
      if (ret != 0) {
        AV_LOGW(LOG_TAG, "REDCache - %s fseek to end is failed!\n",
                __FUNCTION__);
        return -1;
      }
    }
  } else {
    // AV_LOGW(LOG_TAG, "REDCache - %s add new file - %s\n", __FUNCTION__,
    // uri.c_str());
    const std::string local_path = base_local_path + uri;
    const std::string loacl_path_map = base_local_path + uri + "-map";
    cachePath = new REDCachePath(uri, local_path);
    if (!cachePath) {
      AV_LOGW(LOG_TAG, "%s REDCache new cache path - %s is failed!\n",
              __FUNCTION__, uri.c_str());
      return -1;
    }
    cache_path_map[uri] = cachePath;
    cachePath->mpin = true;
    add_to_head(cachePath);
    cachePath->mfd_map = fopen(loacl_path_map.c_str(), "w+");
    if (cachePath->mfd_map == nullptr) {
      AV_LOGW("REDCache - %s fopen mfd_map failed!\n", __FUNCTION__);
      return -1;
    }
    fclose(cachePath->mfd_map);
    cachePath->mfd_map = nullptr;

    cachePath->mfd = fopen(local_path.c_str(), "w+");
    if (cachePath->mfd == nullptr) {
      AV_LOGW("REDCache - %s fopen mfd failed!\n", __FUNCTION__);
      return -1;
    }
  }
  while (cache_path_map.size() > max_cache_entries ||
         physical_total_size > max_dir_capacity) {
    // AV_LOGW(LOG_TAG, "REDCache - %s cache_path_map size:%d
    // max_cache_entries:%d, dirsize %" PRId64 ", maxdircpacity %" PRId64
    // "\n",
    //     __FUNCTION__, cache_path_map.size(), max_cache_entries,
    //     physical_total_size, max_dir_capacity);
    if (!delete_local_file()) {
      break;
    }
  }
  if (cachePath != nullptr && cachePath->mfd != nullptr && amount_data > 0) {
    size_t readsize = fread(data, 1, amount_data, cachePath->mfd);
    if (readsize < 0) {
      AV_LOGW(LOG_TAG,
              "REDCache - %s fread file is failed! amountdata %d, "
              "readsize %zu\n",
              __FUNCTION__, amount_data, readsize);
      return -1;
    }
  }
  return amount_data;
}

void REDFileCache::close_cache_file(const std::string &uri) {
  std::lock_guard<std::mutex> lock(map_mutex);
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  REDCachePath *cache_path = search_cache(uri);
  if (cache_path == nullptr) {
    AV_LOGW(LOG_TAG, "REDCache - %s cache_path - %s is null\n", __FUNCTION__,
            uri.c_str());
    return;
  }

  save_cache_info(cache_path);
  if (cache_path->mfd_map != nullptr) {
    fclose(cache_path->mfd_map);
    cache_path->mfd_map = nullptr;
  }

  if (cache_path->mfd != nullptr) {
    fclose(cache_path->mfd);
    cache_path->mfd = nullptr;
    cache_path->mpin = false;
    // AV_LOGW(LOG_TAG, "REDCache - %s mpin = false uri = %s\n",
    // __FUNCTION__, uri.c_str());
  }
  // AV_LOGW(LOG_TAG, "REDCache - %s mpin is:%d uri = %s\n", __FUNCTION__,
  // cache_path->mpin, uri.c_str());
}

void REDFileCache::cache_path_init() {
  std::string dummystr("dummy");
  cache_path_head = new REDCachePath(dummystr, dummystr);
  cache_path_tail = new REDCachePath(dummystr, dummystr);
  if (!cache_path_head || !cache_path_tail) {
    AV_LOGW(LOG_TAG, "REDCache - %s cache_path_head:%p, cache_path_tail:%p\n",
            __FUNCTION__, cache_path_head, cache_path_tail);
  }
  cache_path_head->next = cache_path_tail;
  cache_path_tail->prev = cache_path_head;
}

void REDFileCache::add_to_head(REDCachePath *path) {
  std::lock_guard<std::mutex> lock(path_mutex);
  path->prev = cache_path_head;
  path->next = cache_path_head->next;
  cache_path_head->next->prev = path;
  cache_path_head->next = path;
}

void REDFileCache::move_to_head(REDCachePath *path) {
  std::lock_guard<std::mutex> lock(path_mutex);
  // remove cache
  path->prev->next = path->next;
  path->next->prev = path->prev;
  // add to head
  path->prev = cache_path_head;
  path->next = cache_path_head->next;
  cache_path_head->next->prev = path;
  cache_path_head->next = path;
}

REDCachePath *REDFileCache::delete_tail_cache() {
  std::lock_guard<std::mutex> lock(path_mutex);
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  if (cache_path_map.empty())
    return nullptr;

  // remove cache
  REDCachePath *tail_path = cache_path_tail->prev;
  for (int i = 0; i < cache_path_map.size(); ++i) {
    if (!tail_path) {
      AV_LOGW("REDCache - %s tail_path is nullptr!\n", __FUNCTION__);
      return nullptr;
    }
    if (!tail_path->prev || !tail_path->next) {
      AV_LOGW("REDCache - %s tail_path->prev:%p, tail_path->next:%p\n",
              __FUNCTION__, tail_path->prev, tail_path->next);
      return nullptr;
    }
    if (!tail_path->mpin) {
      tail_path->prev->next = tail_path->next;
      tail_path->next->prev = tail_path->prev;
      return tail_path;
    }
    tail_path = tail_path->prev;
  }

  return nullptr;
}

REDCachePath *REDFileCache::delete_cache_by_uri(const std::string uri) {
  std::lock_guard<std::mutex> lock(path_mutex);
  // AV_LOGW(LOG_TAG, "REDCache - %s\n", __FUNCTION__);
  if (cache_path_map.empty())
    return nullptr;

  // remove cache
  REDCachePath *tail_path = cache_path_tail->prev;
  for (int i = 0; i < cache_path_map.size(); ++i) {
    if (!tail_path) {
      AV_LOGW("REDCache - %s tail_path is nullptr!\n", __FUNCTION__);
      return nullptr;
    }
    if (!tail_path->prev || !tail_path->next) {
      AV_LOGW("REDCache - %s tail_path->prev:%p, tail_path->next:%p\n",
              __FUNCTION__, tail_path->prev, tail_path->next);
      return nullptr;
    }
    if (!tail_path->mpin &&
        tail_path->key_url == uri) { // todo(renwei) make sure of this condition
      tail_path->prev->next = tail_path->next;
      tail_path->next->prev = tail_path->prev;
      return tail_path;
    }
    tail_path = tail_path->prev;
  }

  return nullptr;
}

void REDFileCache::get_all_cache_files(const std::string &dirpath,
                                       char ***cached_file,
                                       int *cached_file_len) {
  std::lock_guard<std::mutex> lock(map_mutex);
  std::unordered_map<std::string, REDCachePath *>::iterator mapIter;
  *cached_file_len = cache_path_map.size();
  (*cached_file) =
      reinterpret_cast<char **>(malloc(*cached_file_len * sizeof(char *)));
  int i;
  for (mapIter = cache_path_map.begin(), i = 0; mapIter != cache_path_map.end();
       ++mapIter, i++) {
    *((*cached_file) + i) = strdup(mapIter->second->key_url.c_str());
  }
}

void REDFileCache::delete_cache_file(const std::string &uri) {
  std::lock_guard<std::mutex> lock(map_mutex);
  delete_local_file(uri);
}
