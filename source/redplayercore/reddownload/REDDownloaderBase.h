#pragma once

#include "REDDownloader.h"
#include "utility/Utility.h"

enum ErrorBase {
  OpenError = -11000,
  GetDataError = -21000,
  GetDataTimeoutError = -31000,
  CallbackError = -32000,
  CallbackFileLengthError = -33000,
  RundownloadRangeError = -41000,
  RundownloadNullError = -42000,
  PreCreateTaskError = -51000,
  PreCreateTaskNullError = -52000,
};

enum ErrorSwitch {
  SWITCH_BACK_TO_CDN = -2,
};

class RedDownloadBase : public RedDownLoad {
public:
  RedDownloadBase(Source source, int size);
  ~RedDownloadBase();
  std::shared_ptr<RedDownloadStatus> getdownloadstatus() override {
    return mdownloadStatus;
  }

protected:
  void resetDownloadStatus();
  using ErrorData = int;
  void eventCallback(int event,
                     void *data); // TODO: 1.int -> enum class 2.void* ->
                                  // std::any/boost::any

protected:
  // Subclass's parameter
  const Source msource;

  std::atomic_bool bpause{false};
  RedDownLoadPara *mdownpara{nullptr};
  std::shared_ptr<RedDownloadStatus> mdownloadStatus;
  // tcp
  int64_t mtcpStartTs{0};
  // http
  int64_t mhttpStartTs{0};
  // on-off
  bool m_datacallback_crash{false};
  int64_t mhttpFirstDataCallabckTs{0};

  int mPlayerGetDataSize{0};
};
