#pragma once

#include <stdint.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "REDDownloadListen.h"
#include "REDDownloadTask.h"
#include "REDDownloaderFactory.h"
#include "REDFileManager.h"
#include "REDThreadPool.h"
#include "REDURLParser.h"
#include "utility/Cipher.h"

class RedDownloadCache : public DataCallBack,
                         public std::enable_shared_from_this<RedDownloadCache> {
public:
  explicit RedDownloadCache(const std::string &url);
  ~RedDownloadCache();
  int64_t Open(const std::string &url, DownLoadListen *cb);
  int64_t GetUid();
  bool Compareurl(const std::string &url);
  void ReadDns();
  int Read(uint8_t *buf, size_t nbyte);
  bool setOpt(DownLoadOpt *option);
  int64_t Seek(int64_t offset, int whence);
  int64_t GetRedDownloadSize(const std::string &url);
  void LoadCache();
  void Stop();
  void Pause();
  void Close();
  void UpdatePreloadTaskPriority();
  void SetDownloadCdn(int download_cdn);
  std::string getUri();

  static void treatTokenInfoVerify(std::string token);
  void getAllCachedFile(const std::string &dirpath, char ***cached_file,
                        int *cached_file_len);
  void deleteCache(const std::string &dirpath, const std::string &uri);

private:
  int ReadAsync(uint8_t *buf, size_t nbyte);
  int Readsync(uint8_t *buf, size_t nbyte);
  size_t WriteData(uint8_t *ptr, size_t size, void *userdata, int serial,
                   int err_code = 0);
  void DownloadCallBack(int what, void *arg1, void *arg2, int64_t arg3,
                        int64_t arg4);
  int InterruptCallBack();
  size_t loadfromfile(int64_t offset, bool needdownload = true);
  size_t loadtofile(bool data_error = false);
  bool CreateTask();
  bool updateTask();
  void updatepara();
  int PreLoad(int64_t nbytes);
  void SortUrlList();

  std::string murl;
  std::list<std::string> murllist;
  std::string mrealurl;
  std::string muri;
  REDFileManager *mfc;

  int64_t mfilesize;
  int64_t mlogicalpos;
  int64_t mdownloadsize;
  int64_t mloadfilepos;
  int64_t mserial;
  int64_t mpreloadsize;
  int mrangesize;

  uint8_t *mbuf;
  std::atomic_bool bload;
  int mbufrpos;
  int mbufwpos;
  std::recursive_mutex m_mutex;
  std::condition_variable_any m_cachecond;

  REDThreadPool *mthreadpool = nullptr;
  std::shared_ptr<REDDownLoadTask> mtask;
  DownLoadListen *mdownloadcb = nullptr;
  DownLoadOpt *moption = nullptr;
  RedDownLoadPara *mdownloadpara = nullptr;
  bool babort;
  bool bpause;
  int merrcode;
  static int64_t uid;
  int64_t muid;
  int tcount;
  bool bupdateparam{true};
  bool neednotify{false};
  bool m_first_load_to_file{
      true}; // first time to load into file per network request
  bool m_datacallback_crash{false};
  bool m_abort_fix{false};
#pragma mark - Decrypt data
private:
  void treatTokenInfo(std::string token);
#pragma pack(1)
  typedef struct TokenInfo {
    uint32_t rangeStart{0};
    uint32_t rangeStop{0};
    uint8_t iv[16]{0};
    uint8_t key[16]{0};
    Cipher::CipherType cipherType{CipherType::NONE};
  } TokenInfo;
#pragma pack()
  TokenInfo mtokenInfo;
  uint8_t *mencryptedBlock{nullptr};
  ByteArray mpredecryptedBlock; // redundant data before the decrypted block
  std::atomic_bool m_download_from_cdn{true};
  int mbuf_extra_size{100 * 1024}; // should at least >= 2*filldata_size
};
