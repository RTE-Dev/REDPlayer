//
//  NetworkQuality.cpp
//  RedDownload
//
//  Created by xhs_zgc on 2023/2/8.
//

#include "NetworkQuality.h"

#include <cmath>
#include <inttypes.h>
#include <mutex>

#include "RedLog.h"
#include "utility/Utility.h"

static std::once_flag networkQualityOnceFlag;
static NetworkQuality *networkQualityInstance = nullptr;
NetworkQuality *NetworkQuality::getInstance() {
  std::call_once(networkQualityOnceFlag, [&] {
    networkQualityInstance = new (std::nothrow) NetworkQuality;
  });
  return networkQualityInstance;
}

#pragma mark Constructor
NetworkQuality::NetworkQuality() {
  // default grading system
  _score.tcp = {
      {{0, 50}, {90, 100}}, {{50, 300}, {60, 90}}, {{300, 3000}, {0, 60}}};
  _score.http = {
      {{0, 50}, {90, 100}}, {{50, 300}, {60, 90}}, {{300, 3000}, {0, 60}}};
  _score.level = {{{90, 100}, NQLevel::EXCELLENT},
                  {{80, 90}, NQLevel::GOOD},
                  {{0, 80}, NQLevel::WEAK},
                  {{-1, 0}, NQLevel::OFFLINE}};
}

#pragma mark Public Interface

void NetworkQuality::addIndicator(
    const std::shared_ptr<NQIndicator> indicator) {
  std::lock_guard<std::mutex> lock(_indicator.mutex);
  // TODO: more strategies
  if (indicator->exception != NQException::NONE) {
    AV_LOGI(NQ_TAG, "%s tcpRTT:%d httpRTT:%d exception:%d\n", __FUNCTION__,
            indicator->tcpRTT, indicator->httpRTT, indicator->exception);
    _indicator.map.clear();
    return;
  }
  int64_t nowTs = getTimestampMs();
  _indicator.map.insert({nowTs, indicator});
  AV_LOGI(NQ_TAG,
          "%s time:%" PRId64 ", tcpRTT:%d httpRTT:%d downloadSpeed:%d\n",
          __FUNCTION__, nowTs, indicator->tcpRTT, indicator->httpRTT,
          indicator->downloadSpeed);
}

std::shared_ptr<NQInfo> NetworkQuality::getNetInfo() {
  std::lock_guard<std::mutex> lock(_indicator.mutex);
  std::shared_ptr<NQInfo> info = std::make_shared<NQInfo>();
  int64_t nowTs = getTimestampMs();
  AV_LOGI(NQ_TAG, "%s nowTs:%" PRId64 "\n", __FUNCTION__, nowTs);
  for (auto iter = _indicator.map.begin(); iter != _indicator.map.end();) {
    if (iter->first < nowTs - _indicator.timeout) {
      AV_LOGI(NQ_TAG, "%s delete ts:%" PRId64 "\n", __FUNCTION__, iter->first);
      iter = _indicator.map.erase(iter);
    } else {
      break;
    }
  }

  int size = static_cast<int>(_indicator.map.size());
  if (size > 4) {
    double tcpRTT = 0;
    double httpRTT = 0;
    double downloadSpeed = 0;

    // TODO: more statistic methods (eg. ES)
    int divisor = 0;
    switch (_score.nPower) {
    case 1:
      divisor = size * (size + 1) / 2;
      break;
    case 2:
      divisor = size * (size + 1) * (2 * size + 1) / 6;
      break;
    default:
      return info;
    }
    int index = 1;
    for (auto iter = _indicator.map.begin(); iter != _indicator.map.end();
         ++iter, ++index) {
      AV_LOGI(NQ_TAG,
              "%s time:%" PRId64 " tcpRTT:%d httpRTT:%d downloadSpeed:%d\n",
              __FUNCTION__, nowTs, iter->second->tcpRTT, iter->second->httpRTT,
              iter->second->downloadSpeed);
      double factor = pow(index, _score.nPower);
      tcpRTT += (iter->second->tcpRTT * factor / divisor);
      httpRTT += (iter->second->httpRTT * factor / divisor);
      downloadSpeed += (iter->second->downloadSpeed * factor / divisor);
    }

    info->level = getLevel(tcpRTT, httpRTT);
    info->downloadSpeed = downloadSpeed;
    AV_LOGI(NQ_TAG, "%s level:%d tcpRTT:%.2f httpRTT:%.2f downloadSpeed:%.2f\n",
            __FUNCTION__, info->level, tcpRTT, httpRTT, downloadSpeed);
  }
  return info;
}

#pragma mark Private method

#pragma mark Score

NQLevel NetworkQuality::getLevel(int tcpRTT, int httpRTT) {
  NQLevel level = NQLevel::NONE;

  double httpScore = 0;
  for (auto iter = _score.http.begin(); iter != _score.http.end(); ++iter) {
    Interval time = iter->first;
    double minTime = std::get<0>(time), maxTime = std::get<1>(time);
    if (httpRTT >= minTime && httpRTT < maxTime) {
      Interval score = iter->second;
      double minScore = std::get<0>(score), maxScore = std::get<1>(score);
      httpScore = minScore + (maxScore - minScore) * (maxTime - httpRTT) /
                                 (maxTime - minTime);
      break;
    }
  }

  double sumScore = 0;
  double tcpScore = 0;
  if (tcpRTT == 0) { // maybe http2.0
    sumScore = httpScore;
  } else {
    for (auto iter = _score.tcp.begin(); iter != _score.tcp.end(); ++iter) {
      Interval time = iter->first;
      double minTime = std::get<0>(time), maxTime = std::get<1>(time);
      if (tcpRTT >= minTime && tcpRTT < maxTime) {
        Interval score = iter->second;
        double minScore = std::get<0>(score), maxScore = std::get<1>(score);
        tcpScore = minScore + (maxScore - minScore) * (maxTime - tcpRTT) /
                                  (maxTime - minTime);
        break;
      }
    }
    sumScore = tcpScore * _score.weight[0] + httpScore * _score.weight[1];
  }

  for (auto iter = _score.level.begin(); iter != _score.level.end(); ++iter) {
    int minScore = std::get<0>(iter->first),
        maxScore = std::get<1>(iter->first);
    if (sumScore > minScore && sumScore <= maxScore) {
      level = iter->second;
      break;
    }
  }

  AV_LOGI(NQ_TAG, "%s sumScore:%.2f tcpScore:%.2f httpScore:%.2f\n",
          __FUNCTION__, sumScore, tcpScore, httpScore);

  return level;
}
