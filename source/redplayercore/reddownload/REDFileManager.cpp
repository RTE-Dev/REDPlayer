#include "REDFileManager.h"

REDFileManager *REDFileManager::minstance = new REDFileManager();
REDFileManager *REDFileManager::getInstance() { return minstance; }

void REDFileManager::delInstance() {
  if (minstance != nullptr) {
    delete minstance;
  }
  minstance = nullptr;
  // AV_LOGW(LOG_TAG, "REDFileManager - %s delete instance\n", __FUNCTION__);
}

REDFileManager::REDFileManager() {
  // AV_LOGW(LOG_TAG, "%s init\n", __FUNCTION__);
}

REDFileManager::~REDFileManager() {
  // AV_LOGW(LOG_TAG, "REDFileManager - %s destroy\n", __FUNCTION__);
}

void REDFileManager::set_dir_capacity(const std::string &path,
                                      std::uint32_t max_entries,
                                      std::int64_t max_capacity,
                                      int downloadcachesize, int cache_type) {
  if (path.empty())
    return;
  std::shared_ptr<REDFileCache> filecache = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex);
    std::unordered_map<std::string, std::shared_ptr<REDFileCache>>::iterator
        mapIter;
    if ((mapIter = cachefilemap.find(path)) != cachefilemap.end()) {
      filecache = cachefilemap[path];
    } else {
      filecache = make_shared<REDFileCache>();
      cachefilemap[path] = filecache;
    }
  }
  filecache->set_cache_type(cache_type);
  filecache->set_cache_path(path);
  if (max_entries > 0) {
    filecache->set_max_cache_entries(max_entries);
  }
  if (max_capacity > 0) {
    filecache->set_max_dir_capacity(max_capacity);
  }
  filecache->set_download_cache_size(downloadcachesize);

  filecache->GetDirectoryFiles();
}

std::shared_ptr<REDFileCache>
REDFileManager::getfilecache(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex);
  if (path.empty())
    return nullptr;
  std::unordered_map<std::string, std::shared_ptr<REDFileCache>>::iterator
      mapIter;
  std::shared_ptr<REDFileCache> filecache = nullptr;
  if ((mapIter = cachefilemap.find(path)) != cachefilemap.end()) {
    filecache = cachefilemap[path];
  }

  return filecache;
}
/*update the download range info of the file(video), when loadtofile*/
int REDFileManager::update_cache_info(const std::string &uri,
                                      const std::string &dirpath,
                                      std::uint8_t *data,
                                      std::int64_t start_pos,
                                      std::uint32_t length) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    return filecache->update_cache_info(uri, data, start_pos, length);
  }
  return -1;
}
/*return the fd, througth the request of the offset*/
int REDFileManager::get_cache_file(const std::string &uri,
                                   const std::string &dirpath,
                                   std::uint8_t *data, std::uint64_t offset,
                                   int bufsize) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    return filecache->get_cache_file(uri, data, offset, bufsize);
  }
  return -1;
}
/*set or get total file size*/
void REDFileManager::set_file_size(const std::string &uri,
                                   const std::string &dirpath,
                                   int64_t filesize) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    filecache->set_file_size(uri, filesize);
  }
}

int64_t REDFileManager::get_file_size(const std::string &uri,
                                      const std::string &dirpath,
                                      int &rangesize) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    return filecache->get_file_size(uri, rangesize);
  }
  return -1;
}
/*get the file cache size*/
int64_t REDFileManager::get_cache_size(const std::string &uri,
                                       const std::string &dirpath) {
  if (!dirpath.empty()) {
    std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
    if (filecache != nullptr) {
      return filecache->get_cache_size(uri);
    }
  } else {
    int64_t cachesize = 0;
    std::lock_guard<std::mutex> lock(mutex);
    for (auto iter = cachefilemap.begin(); iter != cachefilemap.end(); ++iter) {
      cachesize = iter->second->get_cache_size(uri);
      if (cachesize > 0)
        return cachesize;
    }
  }
  return 0;
}

void REDFileManager::close_cache_file(const std::string &uri,
                                      const std::string &dirpath) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    filecache->close_cache_file(uri);
  }
}

void REDFileManager::get_all_cache_files(const std::string &dirpath,
                                         char ***cached_file,
                                         int *cached_file_len) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    filecache->get_all_cache_files(dirpath, cached_file, cached_file_len);
  }
}

void REDFileManager::delete_cache(const std::string &dirpath,
                                  const std::string &uri) {
  std::shared_ptr<REDFileCache> filecache = getfilecache(dirpath);
  if (filecache != nullptr) {
    filecache->delete_cache_file(uri);
  }
}
