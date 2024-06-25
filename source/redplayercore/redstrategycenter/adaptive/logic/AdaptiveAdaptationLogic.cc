#include "adaptive/logic/AdaptiveAdaptationLogic.h"
#include "RedLog.h"
#include "RedStrategyCenterCommon.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include <float.h>
#include <inttypes.h>
#include <string>

namespace redstrategycenter {
namespace adaptive {
namespace logic {

int AdaptiveAdaptationLogic::getInitialRepresentation(
    const std::unique_ptr<playlist::PlayList> &p, const int speed) {
  int ret = -1;
  int max_cached_index = -1;
  int64_t max_cached_size = -1;
  std::string url;
  if (p == nullptr || p->adaptation_set == nullptr) {
    AV_LOGW(RED_STRATEGY_CENTER_TAG, "[%s:%d] failed\n", __FUNCTION__,
            __LINE__);
    return ret;
  }
  if (1 == p->adaptation_set->representations.size()) {
    AV_LOGW(RED_STRATEGY_CENTER_TAG, "[%s:%d]only one url: %s, %dx%d\n",
            __FUNCTION__, __LINE__,
            p->adaptation_set->representations[0]->url.c_str(),
            p->adaptation_set->representations[0]->width,
            p->adaptation_set->representations[0]->height);
    ret = 0;
    return ret;
  }
  for (int i = 0; i < p->adaptation_set->representations.size(); i++) {
    if (p->adaptation_set->representations[i]->url.empty()) {
      AV_LOGW(RED_STRATEGY_CENTER_TAG, "[%s:%d] empty url\n", __FUNCTION__,
              __LINE__);
      continue;
    }
    int64_t cached_size = 0;
    if (reddownload_func_ != nullptr) {
      cached_size =
          reddownload_func_(p->adaptation_set->representations[i]->url.c_str());
    }
    if (cached_size > max_cached_size) {
      max_cached_size = cached_size;
      max_cached_index = i;
    }
    if (ret < 0 &&
        p->adaptation_set->representations[i]->avg_bitrate <= speed) {
      ret = i;
    }
  }
  if (max_cached_index >= 0 && max_cached_size > 0) {
    AV_LOGI(RED_STRATEGY_CENTER_TAG,
            "[%s:%d] get cached url index %d, cached size %" PRId64 "\n",
            __FUNCTION__, __LINE__, max_cached_index, max_cached_size);
    return max_cached_index;
  }
  return ret < 0 ? 0 : ret;
}

int AdaptiveAdaptationLogic::setReddownloadUrlCachedFunc(
    int64_t (*func)(const char *url)) {
  if (func != nullptr) {
    reddownload_func_ = func;
    return 0;
  }
  AV_LOGI(RED_STRATEGY_CENTER_TAG, "[%s:%d] failed\n", __FUNCTION__, __LINE__);
  return -1;
}

} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
