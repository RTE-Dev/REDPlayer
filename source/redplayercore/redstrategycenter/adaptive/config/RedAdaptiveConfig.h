#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace redstrategycenter {
namespace adaptive {
namespace config {
typedef struct ShortVideoConfig {
  ShortVideoConfig();
  int64_t expires_time;
  float ne_scale_factor;
  float ne_percentile;
} ShortVideoConfig;

class RedAdaptiveConfig {
public:
  static RedAdaptiveConfig *getInstance();
  int setShortVideoConfig(const std::string &str);
  int64_t getShortVideoConfigExpiresTime();
  float getShortVideoConfigNeScaleFactor();
  float getShortVideoConfigNePercentile();

private:
  RedAdaptiveConfig();
  ~RedAdaptiveConfig();
  RedAdaptiveConfig(const RedAdaptiveConfig &);
  const RedAdaptiveConfig &operator=(const RedAdaptiveConfig &);

private:
  static RedAdaptiveConfig *instance_;
  std::unique_ptr<ShortVideoConfig> short_video_config_;
  std::mutex config_mutex_;
};
} // namespace config
} // namespace adaptive
} // namespace redstrategycenter
