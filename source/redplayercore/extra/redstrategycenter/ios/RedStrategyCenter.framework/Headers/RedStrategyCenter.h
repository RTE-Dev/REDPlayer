#pragma once

#include "RedStrategyCenterCommon.h"
#include "observer/IDownloadObserver.h"
#include <mutex>
#include <unordered_map>
#include <vector>

#define DEFAULT_MAX_SAMPLES 20

namespace redstrategycenter {
struct Sample {
  Sample(int64_t weight, int64_t value, int64_t time)
      : weight(weight), //每次数据下载大小的平方根,单位:Byte
        value(value),   //平均下载速度,单位:b/s
        time(time) {}   //更新时间,单位:us
  int64_t weight;
  int64_t value;
  int64_t time;
};
class RedStrategyCenter : public observer::IDownloadObserver {
public:
  static RedStrategyCenter *GetInstance();
  void updateDownloadRate(int64_t size, int64_t speed,
                          int64_t cur_time) override;
  void notifyNetworkTypeChanged();
  std::vector<Sample> getSamples();

private:
  RedStrategyCenter();
  ~RedStrategyCenter();
  RedStrategyCenter(const RedStrategyCenter &);
  const RedStrategyCenter &operator=(const RedStrategyCenter &);

private:
  static RedStrategyCenter *instance_;
  std::vector<Sample> samples_;
  std::mutex samples_mutex_;
};
} // namespace redstrategycenter
