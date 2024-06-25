#pragma once

#include "RedStrategyCenter.h"
#include "RedStrategyCenterCommon.h"

namespace redstrategycenter {
namespace evaluate {
class NetworkEvaluate {
public:
  virtual ~NetworkEvaluate() {}
  virtual int64_t getSpeed(float percentile, std::vector<Sample> &samples) = 0;
};
} // namespace evaluate
} // namespace redstrategycenter
