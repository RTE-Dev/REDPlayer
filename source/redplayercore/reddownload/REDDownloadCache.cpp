#include "REDDownloadCache.h"

#include "RedBase.h"
#include "RedDownloadConfig.h"
#include "RedLog.h"
#include "dnscache/REDDnsCache.h"
#include "time.h"
#include <inttypes.h>
#include <unistd.h>

#define LOG_TAG "RedDownloadCache"

#define MAX_SIZE_PER_READ 50 * 1024
#define MAX_MBUF_SIZE 1024 * 1024 * 10
using namespace std;

int64_t RedDownloadCache::uid = 0;
RedDownloadCache::RedDownloadCache(const std::string &url) {
  mfilesize = -1;
  mlogicalpos = 0;
  mloadfilepos = -1;
  mdownloadsize = 0;
  mbufrpos = 0;
  mbufwpos = 0;
  mrangesize = 0;
  babort = false;
  bpause = false;
  mpreloadsize = 0;
  merrcode = 0;
  mserial = 0;
  moption = nullptr;
  mbuf = nullptr;
  mtask = nullptr;
  muid = ++uid;
  tcount = 0;
  mfc = REDFileManager::getInstance();
  murl = url;
  bload.store(false);
  m_datacallback_crash = RedDownloadConfig::getinstance()->get_config_value(
                             FIX_DATACALLBACK_CRASH_KEY) > 0;
  m_abort_fix =
      RedDownloadConfig::getinstance()->get_config_value(ABORT_KEY) > 0;
  int http_range_size =
      RedDownloadConfig::getinstance()->get_config_value(HTTP_RANGE_SIZE);
  int buf_size = http_range_size;
  if (buf_size > 0 && buf_size < MAX_MBUF_SIZE) {
    mbuf_extra_size = buf_size * 2;
  }
}

int64_t RedDownloadCache::GetUid() { return muid; }
RedDownloadCache::~RedDownloadCache() {
  Close();
  mtask = nullptr;
  if (mbuf != nullptr)
    free(mbuf);
  mbuf = nullptr;
  if (moption != nullptr) {
    delete moption;
    moption = nullptr;
  }
  if (mdownloadcb != nullptr) {
    mdownloadcb->DownLoadCb(RED_EVENT_DID_RELEASE, nullptr, nullptr);
    delete mdownloadcb;
    mdownloadcb = nullptr;
  }
  if (mencryptedBlock) {
    free(mencryptedBlock);
    mencryptedBlock = nullptr;
  }
}

void RedDownloadCache::Close() {
  if (!m_abort_fix) {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    babort = true;
  }
  if (mtask != nullptr)
    mtask->stop();
  if (mthreadpool != nullptr)
    mthreadpool->delete_task(mtask);
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  AV_LOGW(LOG_TAG, "%p--RedDownloadCache close, url %s\n", this, murl.c_str());
  if (m_abort_fix) {
    babort = true;
  }
  m_cachecond.notify_one();
  if (bload.load())
    loadtofile();
  if (mfc != nullptr && mbuf != nullptr) {
    if (mfilesize > 0)
      mfc->set_file_size(muri, moption->cache_file_dir, mfilesize);
    mfc->close_cache_file(muri, moption->cache_file_dir);
    free(mbuf);
    mbuf = nullptr;
  }
}

void RedDownloadCache::Stop() {
  {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if (babort) {
      return;
    }
    AV_LOGW(LOG_TAG, "%p--RedDownloadCache Stop, url %s\n", this, murl.c_str());
    babort = true;
    mfilesize = -1;
    m_cachecond.notify_one();
  }
  if (mtask != nullptr)
    mtask->stop();
}

void RedDownloadCache::Pause() {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  if (bpause) {
    return;
  }
  bpause = true;
  AV_LOGW(LOG_TAG, "%p--RedDownloadCache Pause, url %s\n", this, murl.c_str());
  if (mtask != nullptr)
    mtask->pause(false);
}

void RedDownloadCache::LoadCache() {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  AV_LOGW(LOG_TAG, "%p--RedDownloadCache LoadCache, url %s\n", this,
          murl.c_str());
  if (bload.load())
    loadtofile();
  if (mfc != nullptr && mbuf != nullptr) {
    if (mfilesize > 0)
      mfc->set_file_size(muri, moption->cache_file_dir, mfilesize);
    mfc->close_cache_file(muri, moption->cache_file_dir);
    free(mbuf);
    mbuf = nullptr;
  }
}

int64_t RedDownloadCache::Seek(int64_t offset, int whence) {
  {
    if (offset > INT64_MAX / 2)
      return ERROR(EINVAL);
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if ((moption != nullptr) && moption->islive) {
      return ERROR(ENOSYS);
    }
    AV_LOGI(LOG_TAG, "%p--RedDownloadCache seek %" PRId64 ", whence %d\n", this,
            offset, whence);
    if (whence == 65536) {
      return mfilesize;
    }
    if (offset == -1) {
      return ERROR(EINVAL);
    }
    if (offset >= mloadfilepos && offset < mloadfilepos + mrangesize &&
        mbuf != nullptr) {
      mbufrpos = static_cast<int>(offset - mloadfilepos);
      mlogicalpos = offset;
      return offset;
    }
  }
  if ((mtask != nullptr) && (moption->readasync || mpreloadsize > 0)) {
    mtask->flush();
    mpreloadsize = 0;
  }
  {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if (bload.load()) {
      loadtofile();
    }
    mlogicalpos = offset;
    if (moption != nullptr && moption->loadfile) {
      loadfromfile(mlogicalpos, moption->readasync);
    } else {
      if (mbuf == nullptr)
        mbuf =
            reinterpret_cast<uint8_t *>(malloc(mrangesize + mbuf_extra_size));
      if (mbuf) {
        memset(mbuf, 0, mrangesize + mbuf_extra_size);
      }
    }
  }
  if (!moption->readasync) {
    bupdateparam = true;
  }
  return offset;
}

size_t RedDownloadCache::loadfromfile(int64_t offset, bool needdownload) {
  int64_t loadpos = 0;
  mbufrpos = 0;
  mbufwpos = 0;
  int len = 0;
  while (loadpos + mrangesize <= offset) {
    loadpos += mrangesize;
  }
  mloadfilepos = loadpos;
  if (mbuf == nullptr)
    mbuf = reinterpret_cast<uint8_t *>(malloc(mrangesize + mbuf_extra_size));
  if (mbuf) {
    memset(mbuf, 0, mrangesize + mbuf_extra_size);
  }
  if (moption != nullptr && moption->loadfile) {
    len = min(mfc->get_cache_file(muri, moption->cache_file_dir, mbuf,
                                  mloadfilepos, mrangesize),
              mrangesize);
    if (len < 0) {
      AV_LOGE(LOG_TAG, "%p, %s,get_cache_file failed, use data from http\n",
              this, __FUNCTION__);
      len = 0;
    }
  }
  if (!needdownload) {
    mbufwpos = len;
    mbufrpos = static_cast<int>(offset - mloadfilepos);
    bload.store(true);
    AV_LOGI(LOG_TAG, "%p, %s, %" PRId64 ", size %d\n", this, __FUNCTION__,
            mloadfilepos, len);
    return len;
  }
  mserial++;
  if (len == 0) {
    AV_LOGI(LOG_TAG, "%p, %s, %" PRId64 ", size 0 , download from http\n", this,
            __FUNCTION__, mloadfilepos);
    updateTask();
  } else {
    mbufwpos = len;
    if (len < mrangesize &&
        (mfilesize <= 0 || mloadfilepos + len < mfilesize)) {
      updateTask();
    }
    AV_LOGI(LOG_TAG, "%p, %s,  %" PRId64 ", size %d\n", this, __FUNCTION__,
            mloadfilepos, len);
  }
  mbufrpos = static_cast<int>(offset - mloadfilepos);
  bload.store(true);
  return len;
}

bool RedDownloadCache::CreateTask() {
  mthreadpool = REDThreadPool::getinstance();
  // mtask       = mthreadpool->get_task(murl);
  if (mtask == nullptr) {
    mtask = make_shared<REDDownLoadTask>();
    // AV_LOGI(LOG_TAG, "%p %s -%p, create task from threadpool\n", this,
    // __FUNCTION__, mtask.get());
  }
  if (mdownloadpara == nullptr) {
    mdownloadpara = new RedDownLoadPara;
  }
  if (m_datacallback_crash)
    mdownloadpara->mdatacb_weak =
        shared_from_this();      // TODO: C++17, it can be optimised by using
                                 // weak_from_this()
  mdownloadpara->mdatacb = this; // TODO: delete if mdatacb_weak works
  mdownloadpara->mdownloadcb = mdownloadcb;
  mdownloadpara->range_start = mloadfilepos + mbufwpos;
  if (mpreloadsize > 0) {
    mdownloadpara->range_end = mloadfilepos + mpreloadsize - 1;
  } else if (moption->DownLoadType == DOWNLOADADS) {
    mdownloadpara->range_end = 0;
  } else {
    mdownloadpara->range_end = mloadfilepos + mrangesize - 1;
  }
  mdownloadpara->downloadsize = mloadfilepos + mbufwpos;
  mdownloadpara->serial = mserial;
  mdownloadpara->rangesize = mrangesize;
  AV_LOGI(LOG_TAG, "%p %s, range_start %" PRId64 "\n", this, __FUNCTION__,
          mloadfilepos + mbufwpos);
  if (moption == nullptr)
    moption = new DownLoadOpt();
  mdownloadpara->mopt = moption;
  mdownloadpara->url = mrealurl;
  mtask->setparameter(mdownloadpara);
  bool bpreload = moption->PreDownLoadSize > 0;
  mthreadpool->add_task(mtask, bpreload);
  m_first_load_to_file = true;

  return true;
}

bool RedDownloadCache::updateTask() {
  if (mtask == nullptr) {
    CreateTask();
  } else {
    if (mdownloadpara == nullptr) {
      mdownloadpara = new RedDownLoadPara;
      if (m_datacallback_crash)
        mdownloadpara->mdatacb_weak =
            shared_from_this();      // TODO: C++17, it can be optimised by
                                     // using weak_from_this()
      mdownloadpara->mdatacb = this; // TODO: delete if mdatacb_weak works
      mdownloadpara->mdownloadcb = mdownloadcb;
      if (moption == nullptr)
        moption = new DownLoadOpt();
      mdownloadpara->mopt = moption;
    }
    mdownloadpara->range_start = mloadfilepos + mbufwpos;
    if (mpreloadsize > 0) {
      mdownloadpara->range_end = mloadfilepos + mpreloadsize - 1;
    } else if (moption->DownLoadType == DOWNLOADADS) {
      mdownloadpara->range_end = 0;
    } else {
      mdownloadpara->range_end = mloadfilepos + mrangesize - 1;
    }
    mdownloadpara->downloadsize = mloadfilepos + mbufwpos;
    mdownloadpara->serial = mserial;
    mdownloadpara->rangesize = mrangesize;
    mdownloadpara->url = mrealurl;

    AV_LOGI(LOG_TAG, "%p %s, range_start %" PRId64 "\n", this, __FUNCTION__,
            mloadfilepos + mbufwpos);
    mtask->updatepara();
    AV_LOGI(LOG_TAG, "%p %s, %" PRId64 "\n", this, __FUNCTION__,
            mloadfilepos + mbufwpos);
    m_first_load_to_file = true;
  }
  return true;
}

size_t RedDownloadCache::loadtofile(bool data_error) {
  bool notloadtofile = (moption && !moption->loadfile) || data_error;
  if (!notloadtofile && mtask && m_first_load_to_file) {
    notloadtofile |= (mtask->getdownloadstatus()->httpcode >= 400);
    int64_t downloadsize =
        mtask->getdownloadstatus()->downloadsize; // size from network
    if (mbuf != nullptr && downloadsize > 0 && mbufwpos >= downloadsize) {
      std::string bufString(
          reinterpret_cast<char *>(mbuf + mbufwpos - downloadsize),
          downloadsize < 10 * 1024 ? static_cast<size_t>(downloadsize)
                                   : 10 * 1024);
      notloadtofile |= (bufString.find("<html>") != std::string::npos &&
                        bufString.find("</html>") != std::string::npos);
      notloadtofile |= (bufString.find("<?xml version=") != std::string::npos);
    }
  }
  if (notloadtofile) {
    AV_LOGI(LOG_TAG, "%p %s, notloadtofile %" PRId64 ", size %d\n", this,
            __FUNCTION__, mloadfilepos, mbufwpos);
    if (mbuf != nullptr) {
      memset(mbuf, 0, mrangesize + mbuf_extra_size);
    }
    if (m_first_load_to_file && mdownloadsize > 0 && mtask) {
      int64_t downloadsize =
          mtask->getdownloadstatus()->downloadsize; // size from network
      if (downloadsize > 0 && mbufwpos >= downloadsize)
        mdownloadsize -= (mbufwpos - downloadsize);
      mfilesize = -1;
      if (mdownloadpara)
        mdownloadpara->filesize = 0;
      if (mfc)
        mfc->set_file_size(muri, moption->cache_file_dir, 0);
    }
    mbufrpos = 0;
    mbufwpos = 0;
    bload.store(false);
    return mbufwpos;
  }
  if (mfc == nullptr || mbuf == nullptr) {
    AV_LOGW(LOG_TAG, "%p, %s, failed\n", this, __FUNCTION__);
    return 0;
  }
  AV_LOGI(LOG_TAG, "%p %s, loadtofile %" PRId64 ", size %d\n", this,
          __FUNCTION__, mloadfilepos, mbufwpos);

  if (mbufwpos > mrangesize)
    mfc->update_cache_info(muri, moption->cache_file_dir, mbuf, mloadfilepos,
                           mrangesize);
  else
    mfc->update_cache_info(muri, moption->cache_file_dir, mbuf, mloadfilepos,
                           mbufwpos);
  mbufrpos = 0;
  mbufwpos = 0;
  bload.store(false);
  if (mtask && mtask->getdownloadstatus()->downloadsize > 0)
    m_first_load_to_file = false;
  return mbufwpos;
}

// Get a point position relative to other two points
// -1: point < min(point1, point2)
// 0: point between point1 and point2
// 1: point > max(point1, point2)
static int pointRelativePos(int64_t point, int64_t point1, int64_t point2) {
  if (point1 > point2) {
    swap(point1, point2);
  }
  if (point < point1) {
    return -1;
  } else if (point > point2) {
    return 1;
  }
  return 0;
}

size_t RedDownloadCache::WriteData(uint8_t *ptr, size_t size, void *userdata,
                                   int serial, int err_code) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  if (babort || bpause) {
    return size - 1;
  }

  RedDownLoadPara *mdownpara = reinterpret_cast<RedDownLoadPara *>(userdata);
  if (err_code != 0) {
    if (err_code == EOF && mdownpara != nullptr) {
      if (mfilesize != mbufwpos + mdownpara->range_start) {
        AV_LOGI(LOG_TAG,
                "%p %s,eos reached, file size %" PRId64 " -- %" PRId64 "\n",
                this, __FUNCTION__, mfilesize,
                mbufwpos + mdownpara->range_start);
        merrcode = EOF;
      }
      if (mpreloadsize > 0 || moption->DownLoadType == DOWNLOADADS) {
        if (bload.load() && (moption->PreDownLoadSize > 0 ||
                             moption->DownLoadType == DOWNLOADADS))
          loadtofile();
        if (mfc != nullptr && mbuf != nullptr) {
          if (mfilesize > 0)
            mfc->set_file_size(muri, moption->cache_file_dir, mfilesize);
          mfc->close_cache_file(muri, moption->cache_file_dir);
          free(mbuf);
          mbuf = nullptr;
        }
        AV_LOGI(LOG_TAG, "%p %s, pre download finished\n", this, __FUNCTION__);
        mdownpara->preload_finished = true;
        mpreloadsize = 0;
      }
    } else if (mdownpara != nullptr) {
      AV_LOGI(LOG_TAG,
              "%p %s,errorcode %d, file size %" PRId64 " -- %" PRId64 "\n",
              this, __FUNCTION__, err_code, mfilesize,
              mbufwpos + mdownpara->range_start);
      merrcode = err_code;
    }
    m_cachecond.notify_one();
    return size;
  } else if (mbuf == nullptr) {
    merrcode = EINVAL;
    m_cachecond.notify_one();
    return size;
  }
  if (mdownpara != nullptr) {
    if ((mfilesize <= 0) && (mdownpara->filesize > 0)) {
      mfilesize = mdownpara->filesize;
      if ((mfilesize > 0) && (mfc != nullptr) && mdownpara->mopt->loadfile) {
        mfc->set_file_size(muri, moption->cache_file_dir, mfilesize);
      }
      AV_LOGI(LOG_TAG, "%p %s,get file size %" PRId64 "\n", this, __FUNCTION__,
              mfilesize);
    }

    if (serial != mserial) {
      AV_LOGW(LOG_TAG, "%p %s,err serial  %d-%" PRId64 "\n", this, __FUNCTION__,
              serial, mserial);
      return size - 1;
    }
  }
  uint8_t *data = ptr;
  int dataSize = static_cast<int>(size);
  uint8_t *decryptedData = nullptr;
  while (mtokenInfo.cipherType != CipherType::NONE && mencryptedBlock) {
    int64_t dataRangeEnd =
        mdownpara->range_start + mtask->getdownloadstatus()->downloadsize;
    int64_t dataRangeStart = dataRangeEnd - dataSize;
    AV_LOGI(LOG_TAG,
            "%p %s, decrypt info, token range: %u-%u, data range: %" PRId64
            "-%" PRId64 "\n",
            this, __FUNCTION__, mtokenInfo.rangeStart, mtokenInfo.rangeStop,
            dataRangeStart, dataRangeEnd);
    if (dataRangeStart >= mtokenInfo.rangeStop ||
        dataRangeEnd <= mtokenInfo.rangeStart) {
      break;
    }
    if (pointRelativePos(dataRangeStart, mtokenInfo.rangeStart,
                         mtokenInfo.rangeStop) == -1 &&
        pointRelativePos(dataRangeEnd, mtokenInfo.rangeStart,
                         mtokenInfo.rangeStop) == 0) {
      memcpy(mencryptedBlock, data + mtokenInfo.rangeStart - dataRangeStart,
             dataRangeEnd - mtokenInfo.rangeStart);
      mpredecryptedBlock = ByteArray(
          data, static_cast<int>(mtokenInfo.rangeStart - dataRangeStart));
      if (mpredecryptedBlock.size == 0) {
        AV_LOGI(LOG_TAG, "%p %s, mpredecryptedBlock is not valid for case 1\n",
                this, __FUNCTION__);
        return size - 1;
      }
    } else if (pointRelativePos(dataRangeStart, mtokenInfo.rangeStart,
                                mtokenInfo.rangeStop) == -1 &&
               pointRelativePos(dataRangeEnd, mtokenInfo.rangeStart,
                                mtokenInfo.rangeStop) == 1) {
      memcpy(mencryptedBlock, data + mtokenInfo.rangeStart - dataRangeStart,
             mtokenInfo.rangeStop - mtokenInfo.rangeStart);
      mpredecryptedBlock = ByteArray(
          data, static_cast<int>(mtokenInfo.rangeStart - dataRangeStart));
      if (mpredecryptedBlock.size == 0) {
        AV_LOGI(LOG_TAG, "%p %s, mpredecryptedBlock is not valid for case 2\n",
                this, __FUNCTION__);
        return size - 1;
      }
    } else if (pointRelativePos(dataRangeStart, mtokenInfo.rangeStart,
                                mtokenInfo.rangeStop) == 0 &&
               pointRelativePos(dataRangeEnd, mtokenInfo.rangeStart,
                                mtokenInfo.rangeStop) == 0) {
      memcpy(mencryptedBlock + dataRangeStart - mtokenInfo.rangeStart, data,
             dataRangeEnd - dataRangeStart);
    } else if (pointRelativePos(dataRangeStart, mtokenInfo.rangeStart,
                                mtokenInfo.rangeStop) == 0 &&
               pointRelativePos(dataRangeEnd, mtokenInfo.rangeStart,
                                mtokenInfo.rangeStop) == 1) {
      memcpy(mencryptedBlock + dataRangeStart - mtokenInfo.rangeStart, data,
             mtokenInfo.rangeStop - dataRangeStart);
    }
    if (pointRelativePos(mtokenInfo.rangeStop, dataRangeStart, dataRangeEnd) ==
        0) {
      Cipher cipher;
      cipher.setData(
          {mencryptedBlock,
           static_cast<int>(mtokenInfo.rangeStop - mtokenInfo.rangeStart)});
      ByteArray bytes = cipher.Decrypt(mtokenInfo.cipherType,
                                       {mtokenInfo.key, sizeof(mtokenInfo.key)},
                                       {mtokenInfo.iv, sizeof(mtokenInfo.iv)});
      if (bytes.size == 0) {
        AV_LOGI(LOG_TAG, "%p %s,  cipher invalid\n", this, __FUNCTION__);
        return size - 1;
      }
      dataSize = mpredecryptedBlock.size + bytes.size +
                 static_cast<int>(dataRangeEnd - mtokenInfo.rangeStop);
      decryptedData = reinterpret_cast<uint8_t *>(malloc(dataSize));
      if (!decryptedData) {
        AV_LOGI(LOG_TAG, "%p %s,  decryptedData invalid\n", this, __FUNCTION__);
        return size - 1;
      } else {
        AV_LOGI(LOG_TAG, "%p %s, successfully decrypted\n", this, __FUNCTION__);
      }
      if (mpredecryptedBlock.data) {
        memcpy(decryptedData, mpredecryptedBlock.data, mpredecryptedBlock.size);
      }
      memcpy(decryptedData + mpredecryptedBlock.size, bytes.data, bytes.size);
      memcpy(decryptedData + mpredecryptedBlock.size + bytes.size,
             ptr + mtokenInfo.rangeStop - dataRangeStart,
             dataRangeEnd - mtokenInfo.rangeStop);
      data = decryptedData;
      free(mencryptedBlock);
      mencryptedBlock = nullptr;
    } else {
      AV_LOGI(LOG_TAG, "%p %s, token end not reaching, return\n", this,
              __FUNCTION__);
      return size;
    }

    break;
  }

  int write_size = min(static_cast<int>(dataSize), mrangesize - mbufwpos);
  if (write_size > 0) {
    memcpy(mbuf + mbufwpos, data, write_size);
    mbufwpos += write_size;
    if ((moption != nullptr) && moption->islive) {
      // do nothing
    } else {
      mdownloadsize += write_size;
      mdownpara->downloadsize += write_size;
    }
  } else {
    write_size = 0;
  }
  if (mpreloadsize > 0 || moption->DownLoadType == DOWNLOADADS) {
    if ((mpreloadsize > 0 && (mloadfilepos + mbufwpos >= mpreloadsize)) ||
        (mfilesize > 0 && mloadfilepos + mbufwpos >= mfilesize)) {
      if (bload.load() && (moption->PreDownLoadSize > 0 ||
                           moption->DownLoadType == DOWNLOADADS))
        loadtofile();
      if (mfc != nullptr && mbuf != nullptr) {
        if (mfilesize > 0)
          mfc->set_file_size(muri, moption->cache_file_dir, mfilesize);
        mfc->close_cache_file(muri, moption->cache_file_dir);
        free(mbuf);
        mbuf = nullptr;
      }
      AV_LOGI(LOG_TAG, "%p %s, pre download finished\n", this, __FUNCTION__);
      mdownpara->preload_finished = true;
      m_cachecond.notify_one();
      if (decryptedData) {
        free(decryptedData);
        decryptedData = nullptr;
      }
      if (moption->DownLoadType == DOWNLOADADS) {
        return write_size > size ? size : write_size;
      } else if (mloadfilepos + mbufwpos > mpreloadsize) {
        mpreloadsize = 0;
        return write_size - 1;
      }
      mpreloadsize = 0;
      return write_size > size ? size : write_size;
    } else if (mbufwpos >= mrangesize) {
      if (bload.load() && (moption->PreDownLoadSize > 0 ||
                           moption->DownLoadType == DOWNLOADADS))
        loadtofile();
      loadfromfile(mloadfilepos + mrangesize, false);
      mbufwpos = 0;
      int leftsize = min(static_cast<int>(dataSize - write_size), mrangesize);
      if (leftsize > 0) {
        memcpy(mbuf, data + write_size, leftsize);
        mbufwpos += leftsize;
        write_size += leftsize;
      }
    }
  } else if (mpreloadsize == 0 && (!mdownpara->mopt->readasync)) {
    int leftsize = min(static_cast<int>(dataSize - write_size),
                       mrangesize + mbuf_extra_size - mbufwpos);
    if (leftsize > 0) {
      memcpy(mbuf + mbufwpos, data + write_size, leftsize);
      mbufwpos += leftsize;
      if ((moption != nullptr) && moption->islive) {
        // do nothing
      } else {
        mdownloadsize += leftsize;
        mdownpara->downloadsize += leftsize;
      }
      write_size += leftsize;
    }
  }

  m_cachecond.notify_one();
  if (decryptedData) {
    free(decryptedData);
    decryptedData = nullptr;
  }
  return write_size > size ? size : write_size;
}

void RedDownloadCache::SortUrlList() {
  string lasthost = REDDnsCache::getinstance()->GetValidHost();
  if (!lasthost.empty()) {
    auto iter = murllist.begin();
    ++iter;
    for (; iter != murllist.end(); ++iter) {
      std::string::size_type pos = 0;
      if ((pos = iter->find(lasthost)) != std::string::npos) {
        iter_swap(murllist.begin(), iter);
        AV_LOGI(LOG_TAG, "%p %s, swap urllist to %s\n", this, __FUNCTION__,
                lasthost.c_str());
        return;
      }
    }
  }
}

int64_t RedDownloadCache::Open(const std::string &url, DownLoadListen *cb) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  murl = url;
  if (moption != nullptr && moption->use_https) {
    std::string::size_type pos = 0;
    std::string subString = "http://";
    while ((pos = murl.find(subString)) != std::string::npos) {
      murl.replace(pos, subString.length(), "https://");
    }
  }

  if (moption != nullptr && !moption->url_list_separator.empty() &&
      murllist.empty()) {
    UrlParser urlParser(murl, moption->url_list_separator);
    murllist = urlParser.getUrlList();
    if (murllist.empty()) {
      AV_LOGE(LOG_TAG, "%p %s, murllist is empty!\n", this, __FUNCTION__);
      return -1;
    }
    if (RedDownloadConfig::getinstance()->get_config_value(USE_DNS_CACHE) > 0)
      SortUrlList();
    std::list<std::string> tmpList(murllist);
    for (auto &u : tmpList) {
      if (moption != nullptr && moption->use_https) {
        std::string::size_type pos = 0;
        std::string subString = "https://";
        if ((pos = u.find(subString)) != std::string::npos) {
          u.replace(pos, subString.length(), "http://");
        }
      }
      murllist.push_back(u);
    }
    mrealurl = murllist.front();
    murllist.pop_front();
  } else {
    mrealurl = murl;
  }

  if (mdownloadcb != nullptr) {
    delete mdownloadcb;
  }
  mdownloadcb = cb;

  if (moption != nullptr && moption->DownLoadType == DOWNLOADDNS) {
    ReadDns();
    return muid;
  }

  UrlParser up(mrealurl);
  merrcode = 0;
  muri = up.geturi();

  if (muri.empty() || moption == nullptr) {
    AV_LOGI(LOG_TAG, "%p %s, muri or option is empty!\n", this, __FUNCTION__);
    return -1;
  }
  if ((strstr(mrealurl.c_str(), ".flv") != NULL) && (moption != nullptr)) {
    moption->islive = true;
  }
  if (mfilesize <= 0)
    mfilesize = mfc->get_file_size(muri, moption->cache_file_dir, mrangesize);

  if (mdownloadsize == 0) {
    mdownloadsize = mfc->get_cache_size(muri, moption->cache_file_dir);
  }

  moption->cache_size = mdownloadsize;

  RedCacheStatistic *cachedata = new RedCacheStatistic;
  cachedata->cached_size = mdownloadsize;
  cachedata->logical_file_size = mfilesize;
  DownloadCallBack(RED_EVENT_CACHE_STATISTIC,
                   reinterpret_cast<void *>(cachedata), nullptr, 0, 0);
  delete cachedata;
  cachedata = nullptr;

  if (mbuf == nullptr) {
    mbuf = reinterpret_cast<uint8_t *>(malloc(mrangesize + mbuf_extra_size));
    if (mbuf == nullptr) {
      AV_LOGE(LOG_TAG, "%p %s, open malloc buf failed\n", this, __FUNCTION__);
      return ERROR(ENOMEM);
    } else {
      memset(mbuf, 0, mrangesize + mbuf_extra_size);
    }
  }

  if (moption != nullptr &&
      (moption->PreDownLoadSize > 0 || moption->DownLoadType == DOWNLOADADS)) {
    PreLoad(moption->PreDownLoadSize);
  }

  return muid;
}

int64_t RedDownloadCache::GetRedDownloadSize(const std::string &url) {
  int64_t cachesize = 0;
  if (mdownloadsize > 0) {
    cachesize = mdownloadsize;
  } else if (mfc != nullptr) {
    string uri{""};
    if (moption != nullptr && !moption->url_list_separator.empty()) {
      UrlParser up(url, moption->url_list_separator);
      up.geturi();
    } else {
      UrlParser up(url);
      up.geturi();
    }
    if (!uri.empty() && moption) {
      cachesize = mfc->get_cache_size(uri, moption->cache_file_dir);
    }
  }
  AV_LOGW(LOG_TAG, "%s, cachesize %" PRId64 ", downloadsize %" PRId64 "\n",
          __FUNCTION__, cachesize, mdownloadsize);
  return cachesize;
}

bool RedDownloadCache::setOpt(DownLoadOpt *opt) {
  // AV_LOGW(LOG_TAG, "%p %s \n", this, __FUNCTION__);
  mrangesize =
      RedDownloadConfig::getinstance()->get_config_value(DOWNLOADCACHESIZE);
  if (mrangesize <= 0) {
    mrangesize = DOWNLOAD_SHARD_SIZE;
  }
  if (opt == nullptr) {
    AV_LOGW(LOG_TAG, "%s, null option\n", __FUNCTION__);
    return false;
  }
  if (moption != nullptr) {
    // AV_LOGW(LOG_TAG, "%p %s -%p exist !!!\n", this, __FUNCTION__,
    // moption);
    if (moption->headers.empty() && !opt->headers.empty()) {
      moption->headers = opt->headers;
    }
    moption->readasync = opt->readasync;
    moption->loadfile = opt->loadfile;
    moption->PreDownLoadSize = opt->PreDownLoadSize;
    delete opt;
    opt = nullptr;
  } else {
    moption = opt;
  }
  if (moption->cache_file_dir.empty())
    return false;
  if (moption->cache_file_dir.back() != '/') {
    moption->cache_file_dir += '/';
  }
  treatTokenInfo(moption->token);
  mfc->set_dir_capacity(moption->cache_file_dir, moption->cache_max_entries,
                        moption->cache_max_dir_capacity, mrangesize,
                        opt->DownLoadType);
  return true;
}

bool RedDownloadCache::Compareurl(const std::string &url) {
  if (url.empty() || murl.empty())
    return false;
  if (moption != nullptr && !moption->url_list_separator.empty())
    return UrlParser(murl, moption->url_list_separator) ==
           UrlParser(url, moption->url_list_separator);
  return UrlParser(murl) == UrlParser(url);
}

int RedDownloadCache::PreLoad(int64_t nbytes) {
  if ((nbytes > mdownloadsize ||
       (moption->DownLoadType == DOWNLOADADS && nbytes <= 0)) &&
      (mfilesize <= 0 || mfilesize > mdownloadsize)) {
    AV_LOGW(LOG_TAG,
            "%p %s, size %" PRId64 ", download size %" PRId64 ", need load\n",
            this, __FUNCTION__, nbytes, mdownloadsize);
    mpreloadsize = nbytes;
    loadfromfile(mdownloadsize);
    neednotify = true;
  } else {
    UrlParser up(murl);
    std::string cache_path = moption->cache_file_dir + up.geturi();

    RedIOTraffic *mdata = new RedIOTraffic;
    mdata->obj = nullptr;
    mdata->bytes = 0;
    mdata->url = murl;
    mdata->cache_path = cache_path;
    mdata->cache_size = mdownloadsize;
    DownloadCallBack(RED_EVENT_DID_FRAGMENT_COMPLETE,
                     reinterpret_cast<void *>(mdata), nullptr, 0, 0);
    delete mdata;
    mdata = nullptr;
    AV_LOGW(LOG_TAG,
            "%p %s, size %" PRId64 ", download size %" PRId64
            ", has enough buffer\n",
            this, __FUNCTION__, nbytes, mdownloadsize);
  }
  return 0;
}

void RedDownloadCache::ReadDns() {
  RedDownLoadPara *downpara = new RedDownLoadPara;
  if (m_datacallback_crash)
    downpara->mdatacb_weak = shared_from_this();
  downpara->mdatacb = this;
  downpara->mdownloadcb = mdownloadcb;
  downpara->downloadsize = 0;
  downpara->serial = mserial;
  if (moption == nullptr)
    moption = new DownLoadOpt();
  downpara->mopt = moption;
  downpara->url = mrealurl;
  RedDownLoad *downloadhandle = RedDownLoadFactory::create(downpara);
  if (downloadhandle != nullptr) {
    downloadhandle->readdns(downpara);
    delete downloadhandle;
    downloadhandle = nullptr;
  }
  delete downpara;
  downpara = nullptr;
}

int RedDownloadCache::Read(uint8_t *buf, size_t nbyte) {
  int leftsize = static_cast<int>(nbyte);
  int ret;
  if (mfilesize > 0)
    leftsize = leftsize < (mfilesize - mlogicalpos)
                   ? leftsize
                   : static_cast<int>(mfilesize - mlogicalpos);

  if (leftsize <= 0) {
    if (bload.load())
      loadtofile();
    AV_LOGW(LOG_TAG, "%p %s, read EOF\n", this, __FUNCTION__);
    return ERROR_EOF;
  }
  do {
    if (moption != nullptr &&
        (moption->readasync || (mpreloadsize > 0 && mdownloadpara != nullptr &&
                                !mdownloadpara->preload_finished))) {
      ret = ReadAsync(buf, leftsize);
      if (ret == 0)
        usleep(10000);
    } else {
      ret = Readsync(buf, leftsize);
    }
  } while (ret == 0 && !babort && !InterruptCallBack());

  if ((ret < 0) && (moption != nullptr) && moption->islive) {
    {
      std::unique_lock<std::recursive_mutex> lock(m_mutex);
      loadtofile();
      bupdateparam = true;
    }
    return ERROR(EIO);
  }

  bool data_error = false;
  if (mtask && m_first_load_to_file) {
    int64_t downloadsize =
        mtask->getdownloadstatus()->downloadsize; // size from network
    if (mbuf != nullptr && downloadsize > 0 && mbufwpos >= downloadsize) {
      std::string bufString(
          reinterpret_cast<char *>(mbuf + mbufwpos - downloadsize),
          downloadsize < 10 * 1024 ? static_cast<size_t>(downloadsize)
                                   : 10 * 1024);
      data_error |= (bufString.find("<html>") != std::string::npos &&
                     bufString.find("</html>") != std::string::npos);
      data_error |= (bufString.find("<?xml version=") != std::string::npos);
    }
    if (data_error) {
      AV_LOGW(LOG_TAG, "%p %s, data error\n", this, __FUNCTION__);
      if (downloadsize > 0 && mbufwpos >= downloadsize)
        mbufwpos -= downloadsize;
      else
        mbufwpos = 0;
      ret = ERROR(EIO);
    }
  }

  if (ret < 0 && bload.load() && ret != ERROR(EAGAIN)) {
    mpreloadsize = 0;
    if (moption != nullptr && moption->readasync && mtask != nullptr) {
      mtask->flush();
      bupdateparam = true;
    } else if (moption != nullptr && !moption->readasync) {
      bupdateparam = true;
    }
    {
      std::unique_lock<std::recursive_mutex> lock(m_mutex);
      loadtofile(data_error);
    }
    if (moption != nullptr && !moption->url_list_separator.empty() && mtask) {
      std::shared_ptr<RedUrlChangeEvent> urlChangeEvent =
          std::make_shared<RedUrlChangeEvent>();
      urlChangeEvent->current_url = mrealurl;
      std::shared_ptr<RedDownloadStatus> status = mtask->getdownloadstatus();
      if (status)
        urlChangeEvent->http_code = status->httpcode;
      if (!murllist.empty()) {
        urlChangeEvent->next_url = murllist.front();
        mrealurl = murllist.front();
        murllist.pop_front();
        ret = ERROR(EAGAIN);
      }
      DownloadCallBack(RED_EVENT_URL_CHANGE,
                       reinterpret_cast<void *>(urlChangeEvent.get()), nullptr,
                       0, 0);
      AV_LOGE(LOG_TAG,
              "%p %s RED_EVENT_URL_CHANGE now:%s next:%s httpcode:%d\n", this,
              __FUNCTION__, urlChangeEvent->current_url.c_str(),
              urlChangeEvent->next_url.c_str(), urlChangeEvent->http_code);
    }
    if (ret == EOF || ret == ERROR(ETIMEDOUT)) {
      if (ret == ERROR(ETIMEDOUT) && mtask &&
          mtask->getdownloadstatus()->downloadsize == 0) {
        ret = ERROR_TCP_CONNECT_TIMEOUT;
      } else {
        ret = ERROR(EAGAIN);
      }
    }

    tcount = 0;
  }
  merrcode = 0;

  return ret;
}

void RedDownloadCache::UpdatePreloadTaskPriority() {
  if ((mthreadpool == nullptr) || (mtask == nullptr) ||
      (mdownloadpara == nullptr)) {
    return;
  }
  if (mdownloadpara->preload_finished) {
    return;
  }
  AV_LOGI(LOG_TAG, "%p %s task:%p", this, __FUNCTION__, mtask.get());
  mthreadpool->move_task_to_head(mtask);
}

int RedDownloadCache::ReadAsync(uint8_t *buf, size_t nbyte) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  bool readasync =
      moption != nullptr &&
      (moption->readasync || (mpreloadsize > 0 && mdownloadpara != nullptr &&
                              !mdownloadpara->preload_finished));
  if (!readasync)
    return 0;

  int readsize = 0;
  int retry_count =
      RedDownloadConfig::getinstance()->get_config_value(RETRY_COUNT);
  if (!bload.load()) {
    if (loadfromfile(mlogicalpos) < 0)
      return ERROR(EIO);
  }
  if (mlogicalpos >= mloadfilepos && mlogicalpos < mloadfilepos + mrangesize) {
    int64_t loadtime = CurrentTimeUs();
    int lastwpos = mbufwpos;
    while (mbufwpos <= mbufrpos) {
      int64_t currentime = CurrentTimeUs();
      m_cachecond.wait_for(lock, std::chrono::milliseconds(10));
      if (babort || InterruptCallBack()) {
        return 0;
      } else if (merrcode == EOF) {
        AV_LOGE(LOG_TAG,
                "%p %s, get error %d, something wrong in download, "
                "retry agin!!!\n",
                this, __FUNCTION__, merrcode);
        return merrcode;
      } else if (merrcode != 0) {
        AV_LOGE(LOG_TAG, "%p %s, get error %d!!!\n", this, __FUNCTION__,
                merrcode);
        return merrcode;
      } else if (currentime > loadtime + 1000000) {
        AV_LOGE(LOG_TAG, "%p %s, timeout , ERROR %d, wpos %d- %d!!!!\n", this,
                __FUNCTION__, merrcode, lastwpos, mbufwpos);
        if (mbufwpos != lastwpos) {
          tcount = 0;
          return ERROR(EAGAIN);
        } else if (++tcount > retry_count && merrcode == 0) {
          return ERROR(ETIMEDOUT);
        } else if (merrcode == 0) {
          return ERROR(EAGAIN);
        } else {
          return merrcode;
        }
      } else if (mfilesize > 0 && mfilesize <= mlogicalpos) {
        return ERROR_EOF;
      }
    }
    readsize = min(static_cast<int>(nbyte), mbufwpos - mbufrpos);
    if (readsize == 0) {
      return 0;
    }
    tcount = 0;
    memcpy(buf, mbuf + mbufrpos, static_cast<size_t>(readsize));
    mbufrpos += readsize;
    mlogicalpos += readsize;
    return readsize;
  } else {
    loadtofile();
  }
  return 0;
}

int RedDownloadCache::Readsync(uint8_t *buf, size_t nbyte) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);

  int readsize = 0;
  int retry_count =
      RedDownloadConfig::getinstance()->get_config_value(RETRY_COUNT);
  if (!bload.load() && moption != nullptr) {
    if (loadfromfile(mlogicalpos, false) < 0)
      return ERROR(EIO);
  }
  if (mbufrpos >= mrangesize) {
    int extrasize = min(mbufwpos - mrangesize, mbuf_extra_size);
    uint8_t *extrabuf = nullptr;
    if (extrasize > 0) {
      extrabuf = reinterpret_cast<uint8_t *>(malloc(mbuf_extra_size));
      if (extrabuf) {
        memset(extrabuf, 0, mbuf_extra_size);
        memcpy(extrabuf, mbuf + mrangesize, extrasize);
      }
    }
    if (bload.load()) {
      loadtofile();
      loadfromfile(mlogicalpos, false);
    }
    if (extrasize > 0) {
      // If cache size enough, use it and reset task; Or keep downloading
      // from network
      if (mbufwpos > extrasize) {
        bupdateparam = true;
      } else {
        if (mbuf && extrabuf) {
          memset(mbuf, 0, mrangesize + mbuf_extra_size);
          memcpy(mbuf, extrabuf, extrasize);
        }
        mbufwpos = extrasize;
      }
      if (extrabuf) {
        free(extrabuf);
        extrabuf = nullptr;
      }
    } else if (mbufwpos > extrasize) {
      bupdateparam = true;
    }
  }

  int64_t loadtime = CurrentTimeUs();
  int lastwpos = mbufwpos;
  while (mbufwpos <= mbufrpos) {
    if (mtask == nullptr) {
      mtask = make_shared<REDDownLoadTask>();
      mtask->setDownloadSource(m_download_from_cdn.load());
      if (bupdateparam) {
        updatepara();
      }
      mtask->setparameter(mdownloadpara);
    }
    if (bupdateparam) {
      updatepara();
      int ret = mtask->syncupdatepara(neednotify);
      if (ret < 0)
        return ret;
      neednotify = false;
    }

    nbyte = min(static_cast<int>(nbyte), MAX_SIZE_PER_READ);
    bool need_update_range = false;
    int ret = mtask->filldata(nbyte, need_update_range);
    if (ret) {
      if (mbufrpos < mbufwpos) {
        break;
      } else {
        return ret;
      }
    }
    if (ret == 0 && need_update_range == true) {
      bupdateparam = true;
    }
    int64_t currentime = CurrentTimeUs();
    if (currentime > loadtime + 1000000) {
      if (lastwpos != mbufwpos) {
        tcount = 0;
      } else if (++tcount > retry_count) {
        SetDownloadCdn(static_cast<int>(Source::CDN));
        if (mtask != nullptr) {
          std::shared_ptr<RedErrorEvent> errorEvent =
              std::make_shared<RedErrorEvent>();
          int cdn_source = mtask->getDownloadSourceType();
          errorEvent->url = mrealurl;
          errorEvent->error = ETIMEDOUT;
          errorEvent->source = Source(cdn_source);
          DownloadCallBack(RED_EVENT_ERROR,
                           reinterpret_cast<void *>(errorEvent.get()), nullptr,
                           0, 0);
        }
        AV_LOGE(LOG_TAG, "%p %s, timeout!!!!\n", this, __FUNCTION__);
        return ERROR(ETIMEDOUT);
      }
      return ERROR(EAGAIN);
    }
  }
  readsize = min(static_cast<int>(nbyte), mbufwpos - mbufrpos);
  if (readsize > 0) {
    tcount = 0;
    if (mbuf) {
      memcpy(buf, mbuf + mbufrpos, static_cast<size_t>(readsize));
    }
    mbufrpos += readsize;
    mlogicalpos += readsize;
  }
  return readsize;
}

void RedDownloadCache::updatepara() {
  if (mdownloadpara == nullptr) {
    mdownloadpara = new RedDownLoadPara;
  }
  if (moption == nullptr)
    moption = new DownLoadOpt();
  if (m_datacallback_crash)
    mdownloadpara->mdatacb_weak = shared_from_this();
  mdownloadpara->mdatacb = this;
  mdownloadpara->mdownloadcb = mdownloadcb;
  mdownloadpara->serial = mserial;
  mdownloadpara->rangesize = mrangesize;
  if (moption->islive) {
    mdownloadpara->range_start = 0;
    mdownloadpara->range_end = 0;
  } else {
    mdownloadpara->range_start = mloadfilepos + mbufwpos;
    mdownloadpara->range_end = 0;
  }
  AV_LOGI(LOG_TAG, "%p %s, range_start %" PRId64 ", mtask:%p, mbufwpos:%d\n",
          this, __FUNCTION__, mloadfilepos + mbufwpos, mtask.get(), mbufwpos);
  mdownloadpara->mopt = moption;
  mdownloadpara->url = mrealurl;
  if (moption->islive) {
    mdownloadpara->downloadsize = 0;
  } else {
    mdownloadpara->downloadsize = mloadfilepos + mbufwpos;
  }
  m_first_load_to_file = true;
  bupdateparam = false;
}

int RedDownloadCache::InterruptCallBack() {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  if (babort || mdownloadcb == nullptr) {
    return 1;
  }
  return mdownloadcb->InterruptCb();
}

void RedDownloadCache::DownloadCallBack(int what, void *arg1, void *arg2,
                                        int64_t arg3, int64_t arg4) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  if (babort || mdownloadcb == nullptr) {
    return;
  }
  switch (what) {
  case RED_EVENT_WILL_DNS_PARSE:
    mdownloadcb->DownLoadCb(RED_EVENT_WILL_DNS_PARSE, arg1, arg2);
    break;
  case RED_EVENT_DID_DNS_PARSE:
    mdownloadcb->DownLoadCb(RED_EVENT_DID_DNS_PARSE, arg1, arg2);
    break;
  case RED_CTRL_WILL_TCP_OPEN:
    mdownloadcb->DownLoadCb(RED_CTRL_WILL_TCP_OPEN, arg1, arg2);
    break;
  case RED_EVENT_WILL_HTTP_OPEN:
    mdownloadcb->DownLoadCb(RED_EVENT_WILL_HTTP_OPEN, arg1, arg2);
    break;
  case RED_EVENT_DID_HTTP_OPEN:
    mdownloadcb->DownLoadCb(RED_EVENT_DID_HTTP_OPEN, arg1, arg2);
    break;
  case RED_CTRL_DID_TCP_OPEN:
    mdownloadcb->DownLoadCb(RED_CTRL_DID_TCP_OPEN, arg1, arg2);
    break;
  case RED_EVENT_DID_FRAGMENT_COMPLETE:
    mdownloadcb->DownLoadCb(RED_EVENT_DID_FRAGMENT_COMPLETE, arg1, arg2);
    break;
  case RED_EVENT_IO_TRAFFIC:
    mdownloadcb->DownLoadCb(RED_EVENT_IO_TRAFFIC, arg1, arg2);
    break;
  case RED_EVENT_CACHE_STATISTIC:
    mdownloadcb->DownLoadCb(RED_EVENT_CACHE_STATISTIC, arg1, arg2);
    break;
  case RED_EVENT_URL_CHANGE:
    mdownloadcb->DownLoadCb(RED_EVENT_URL_CHANGE, arg1, arg2);
    break;
  case RED_SPEED_CALL_BACK:
    mdownloadcb->SpeedCb(arg3, arg4, CurrentTimeUs());
    break;
  case RED_EVENT_ERROR:
    mdownloadcb->DownLoadCb(RED_EVENT_ERROR, arg1, arg2);
    break;
  case RED_EVENT_IS_PRELOAD:
    mdownloadcb->DownLoadCb(RED_EVENT_IS_PRELOAD, arg1, arg2);
    break;
  case RED_EVENT_FIRST_CATON_OFFSET:
    mdownloadcb->DownLoadCb(RED_EVENT_FIRST_CATON_OFFSET, arg1, arg2);
    break;
  default:
    break;
  }
}

#pragma mark - Decrypt data
void RedDownloadCache::treatTokenInfo(std::string token) {
  if (token.empty())
    return;
  Cipher cipher;
  cipher.setData(token);
  ByteArray decodedBytes = cipher.base64Decode(false);
  if (decodedBytes.size == sizeof(TokenInfo)) {
    memcpy(&mtokenInfo, decodedBytes.data, decodedBytes.size);
  } else {
    AV_LOGW(LOG_TAG, "%p resolve token error\n", this);
  }
  if (mtokenInfo.cipherType != CipherType::NONE) {
    int size = static_cast<int>(mtokenInfo.rangeStop - mtokenInfo.rangeStart);
    if (size > 0 && size < 20 * 1024) { // servers encrypt 16kB data at most
      mencryptedBlock = reinterpret_cast<uint8_t *>(malloc(size));
      mpredecryptedBlock.clear();
    }
  }
}

void RedDownloadCache::SetDownloadCdn(int download_cdn) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  if (download_cdn == static_cast<int>(Source::CDN)) {
    m_download_from_cdn.store(true);
  } else {
    m_download_from_cdn.store(false);
  }
  if (!mtask) {
    AV_LOGI(LOG_TAG, "%p %s,  task is not ready\n", this, __FUNCTION__);
    return;
  }
  if (download_cdn == static_cast<int>(Source::CDN)) {
    mtask->setDownloadSource(true);
  } else {
    mtask->setDownloadSource(false);
  }
}

void RedDownloadCache::treatTokenInfoVerify(std::string token) {
  if (token.empty())
    return;
  Cipher cipher;
  cipher.setData(token);
  ByteArray decodedBytes = cipher.base64Decode(false);
  TokenInfo tokenInfo;
  if (decodedBytes.size == sizeof(TokenInfo)) {
    memcpy(&tokenInfo, decodedBytes.data, decodedBytes.size);
  } else {
    AV_LOGW(LOG_TAG, "%p resolve token error\n", nullptr);
  }
}

std::string RedDownloadCache::getUri() { return muri; }

void RedDownloadCache::getAllCachedFile(const std::string &dirpath,
                                        char ***cached_file,
                                        int *cached_file_len) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  mfc->get_all_cache_files(dirpath, cached_file, cached_file_len);
}

void RedDownloadCache::deleteCache(const std::string &dirpath,
                                   const std::string &uri) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  mfc->delete_cache(dirpath, uri);
}
