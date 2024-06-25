
#include "REDDownloaderFactory.h"

#include <string>

#include "REDCurl.h"
#include "REDDownloader.h"
#include "RedLog.h"

#define LOG_TAG "RedDownloaderFactory"

using namespace std;

RedDownLoad *RedDownLoadFactory::create(RedDownLoadPara *para, CDNType cdn_type,
                                        int range_size) {
  RedDownLoad *reddown = nullptr;
  if (0 == strncmp(para->url.c_str(), "http", 4)) {
    if (cdn_type == CDNType::kNormalCDN) {
      reddown = new RedCurl(Source::CDN, range_size);
      // AV_LOGI(LOG_TAG, "%s, create RedCurl is success!\n",
      // __FUNCTION__);
    }
  }
  return reddown;
}
