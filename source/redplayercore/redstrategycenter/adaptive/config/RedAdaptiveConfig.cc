#include "adaptive/config/RedAdaptiveConfig.h"
#include "RedLog.h"
#include "RedStrategyCenterCommon.h"
#include "json.hpp"

#define SHORT_VIDEO_CONFIG_EXPIRES_TIME_DEFAULT 300000000
#define SHORT_VIDEO_CONFIG_NE_SCALE_FACTOR_DEFAULT 1.0
#define SHORT_VIDEO_CONFIG_NE_PERCENTILE_DEFAULT 0.5

namespace redstrategycenter {
namespace adaptive {
namespace config {

ShortVideoConfig::ShortVideoConfig() {
  expires_time = SHORT_VIDEO_CONFIG_EXPIRES_TIME_DEFAULT;
  ne_scale_factor = SHORT_VIDEO_CONFIG_NE_SCALE_FACTOR_DEFAULT;
  ne_percentile = SHORT_VIDEO_CONFIG_NE_PERCENTILE_DEFAULT;
}

RedAdaptiveConfig *RedAdaptiveConfig::instance_;

RedAdaptiveConfig *RedAdaptiveConfig::getInstance() {
  static std::once_flag oc;
  std::call_once(oc, [&] { instance_ = new RedAdaptiveConfig(); });
  return instance_;
}

RedAdaptiveConfig::RedAdaptiveConfig() {
  short_video_config_.reset(new ShortVideoConfig());
}

RedAdaptiveConfig::~RedAdaptiveConfig() {}

int RedAdaptiveConfig::setShortVideoConfig(const std::string &str) {
  try {
    nlohmann::json j = nlohmann::json::parse(str);
    std::lock_guard<std::mutex> lock(config_mutex_);
    short_video_config_->expires_time = static_cast<int64_t>(
        j.value("expires_time", SHORT_VIDEO_CONFIG_EXPIRES_TIME_DEFAULT));
    short_video_config_->ne_scale_factor = static_cast<float>(
        j.value("ne_scale_factor", SHORT_VIDEO_CONFIG_NE_SCALE_FACTOR_DEFAULT));
    short_video_config_->ne_percentile = static_cast<float>(
        j.value("ne_percentile", SHORT_VIDEO_CONFIG_NE_PERCENTILE_DEFAULT));
    return 0;
  } catch (nlohmann::json::exception &e) {
    return -1;
  } catch (...) {
    return -1;
  }
}

int64_t RedAdaptiveConfig::getShortVideoConfigExpiresTime() {
  std::lock_guard<std::mutex> lock(config_mutex_);
  return short_video_config_->expires_time;
}

float RedAdaptiveConfig::getShortVideoConfigNeScaleFactor() {
  std::lock_guard<std::mutex> lock(config_mutex_);
  return short_video_config_->ne_scale_factor;
}

float RedAdaptiveConfig::getShortVideoConfigNePercentile() {
  std::lock_guard<std::mutex> lock(config_mutex_);
  return short_video_config_->ne_percentile;
}

} // namespace config
} // namespace adaptive
} // namespace redstrategycenter
