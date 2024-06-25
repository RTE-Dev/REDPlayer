#include "RedStrategyCenter.h"
#include "RedBase.h"
#include "RedLog.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include <math.h>
#include <mutex>

namespace redstrategycenter {

RedStrategyCenter *RedStrategyCenter::instance_;

RedStrategyCenter *RedStrategyCenter::GetInstance() {
  static std::once_flag oc;
  std::call_once(oc, [&] { instance_ = new RedStrategyCenter(); });
  return instance_;
}

RedStrategyCenter::RedStrategyCenter() { samples_.clear(); }

RedStrategyCenter::~RedStrategyCenter() { samples_.clear(); }

void RedStrategyCenter::updateDownloadRate(int64_t size, int64_t speed,
                                           int64_t cur_time) {
  std::lock_guard<std::mutex> lock(samples_mutex_);
  if (size > 0 && speed > 0 && cur_time > 0) {
    int64_t weight = (int64_t)sqrt(size);
    struct Sample newSample(weight, speed, cur_time);
    samples_.push_back(newSample);
    while (samples_.size() > DEFAULT_MAX_SAMPLES) {
      samples_.erase(samples_.begin());
    }
  }
}

void RedStrategyCenter::notifyNetworkTypeChanged() {
  std::lock_guard<std::mutex> lock(samples_mutex_);
  samples_.clear();
}

std::vector<Sample> RedStrategyCenter::getSamples() {
  std::lock_guard<std::mutex> lock(samples_mutex_);
  int64_t expires_time = adaptive::config::RedAdaptiveConfig::getInstance()
                             ->getShortVideoConfigExpiresTime();
  int64_t cur_time = CurrentTimeUs();
  while (!samples_.empty() && cur_time > samples_.begin()->time &&
         cur_time - samples_.begin()->time > expires_time) {
    samples_.erase(samples_.begin());
  }
  return samples_;
}

} // namespace redstrategycenter
