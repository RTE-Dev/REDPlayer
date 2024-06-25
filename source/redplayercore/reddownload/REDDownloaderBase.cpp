#include "REDDownloaderBase.h"

#include "RedDownloadConfig.h"

#pragma mark - Constructor/Deconstructor
RedDownLoad::RedDownLoad() {}

RedDownLoad::~RedDownLoad() {}

RedDownloadBase::RedDownloadBase(Source source, int size)
    : msource(source), mPlayerGetDataSize(size) {
  mdownloadStatus = std::make_shared<RedDownloadStatus>();
  m_datacallback_crash = RedDownloadConfig::getinstance()->get_config_value(
                             FIX_DATACALLBACK_CRASH_KEY) > 0;
}

RedDownloadBase::~RedDownloadBase() {}

#pragma mark - Protected
void RedDownloadBase::resetDownloadStatus() {
  mdownloadStatus->httpcode = 0;
  mdownloadStatus->downloadsize = 0;
  mdownloadStatus->errorcode = 0;

  mtcpStartTs = 0;
  mhttpStartTs = 0;
}

void RedDownloadBase::eventCallback(int event, void *data) {
  if (bpause.load() || !mdownpara || !mdownpara->mdatacb) {
    return;
  }

  switch (event) {
  case RED_EVENT_ERROR: {
    std::shared_ptr<RedErrorEvent> errorEvent =
        std::make_shared<RedErrorEvent>();
    errorEvent->url = mdownpara->url;
    errorEvent->error = *(reinterpret_cast<ErrorData *>(data));
    errorEvent->source = msource;
    if (m_datacallback_crash) {
      if (auto dataCallback = mdownpara->mdatacb_weak.lock())
        dataCallback->DownloadCallBack(RED_EVENT_ERROR, errorEvent.get(),
                                       nullptr, 0, 0);
    } else {
      mdownpara->mdatacb->DownloadCallBack(RED_EVENT_ERROR, errorEvent.get(),
                                           nullptr, 0, 0);
    }
  } break;

  default:
    break;
  }
}
