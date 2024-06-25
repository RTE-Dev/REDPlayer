#pragma once

#include "evaluate/NetworkEvaluate.h"
#include <mutex>
#include <stdint.h>
#include <vector>

#define DEFAULT_MAX_WEIGHT 8000
#define DEFAULT_PERCENTILE 0.5

namespace redstrategycenter {
namespace evaluate {
class NetworkEvaluateV1 : public NetworkEvaluate {
public:
  NetworkEvaluateV1(int64_t maxWeight);
  ~NetworkEvaluateV1();
  int64_t getSpeed(float percentile, std::vector<Sample> &samples) override;

private:
  static bool valueComparator(const Sample &a, const Sample &b);

private:
  int64_t mMaxWeight;
};
} // namespace evaluate
} // namespace redstrategycenter
