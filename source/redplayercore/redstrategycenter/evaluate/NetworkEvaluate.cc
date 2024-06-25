#include "RedLog.h"
#include "evaluate/NetworkEvaluateV1.h"
#include <iostream>

#define SORT_ORDER_NONE (-1)
#define SORT_ORDER_BY_VALUE 0
#define SORT_ORDER_BY_INDEX 1

namespace redstrategycenter {
namespace evaluate {

NetworkEvaluateV1::NetworkEvaluateV1(int64_t maxWeight)
    : mMaxWeight(maxWeight) {}

NetworkEvaluateV1::~NetworkEvaluateV1() {}

bool NetworkEvaluateV1::valueComparator(const Sample &a, const Sample &b) {
  return (a.value < b.value);
}

int64_t NetworkEvaluateV1::getSpeed(float percentile,
                                    std::vector<Sample> &samples) {
  int index = 0;
  int64_t totalWeight = 0;
  int64_t desiredWeight = 0;
  int64_t accumulatedWeight = 0;

  for (index = samples.size() - 1; index >= 0; index--) {
    if (totalWeight + samples[index].weight < mMaxWeight) {
      totalWeight += samples[index].weight;
    } else {
      samples[index].weight = mMaxWeight - totalWeight;
      break;
    }
  }
  for (int i = 0; i < index; i++) {
    samples.erase(samples.begin());
  }

  std::sort(samples.begin(), samples.end(), valueComparator);

  if (percentile > 0 && percentile < 1) {
    desiredWeight = (int64_t)(percentile * totalWeight);
  } else {
    // percentile out of range, use default
    AV_LOGW(RED_STRATEGY_CENTER_TAG,
            "[%s:%d] invalid percentile %f, use default\n", __FUNCTION__,
            __LINE__, percentile);
    desiredWeight = (int64_t)(DEFAULT_PERCENTILE * totalWeight);
  }
  for (int i = 0; i < samples.size(); i++) {
    accumulatedWeight += samples[i].weight;
    if (accumulatedWeight >= desiredWeight) {
      return samples[i].value;
    }
  }
  return samples.empty() ? -1 : samples.back().value;
}

} // namespace evaluate
} // namespace redstrategycenter
