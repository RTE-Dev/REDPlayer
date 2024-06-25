#pragma once

#include <map>
#include <mutex>
#include <tuple>
#include <vector>

enum class NQLevel : uint32_t {
  NONE,
  OFFLINE,  // 0
  WEAK,     // (0, 80]
  GOOD,     // (80, 90]
  EXCELLENT // (90, 100]
};

enum class NQException : uint32_t { NONE = 0, DNS, TCP, HTTP, TRANSFER };

typedef struct NQIndicator {
  int tcpRTT;        // ms
  int httpRTT;       // ms
  int downloadSpeed; // B/s
  NQException exception;
  NQIndicator()
      : tcpRTT(0), httpRTT(0), downloadSpeed(0), exception(NQException::NONE) {}
} NQIndicator;

typedef struct NQInfo {
  NQLevel level;
  int downloadSpeed; // predicted
  NQInfo() : level(NQLevel::NONE), downloadSpeed(0) {}
} NQInfo;

class NetworkQuality {
public:
  static NetworkQuality *getInstance();

public:
  void addIndicator(const std::shared_ptr<NQIndicator> indicator);
  std::shared_ptr<NQInfo> getNetInfo();

private:
  NetworkQuality();

private:
  NQLevel getLevel(int tcpRTT, int httpRTT);

private:
  using Interval = std::tuple<int, int>;

private:
  const char *NQ_TAG = "NetworkQuality";
  struct {
    // grading system
    std::map<Interval /*time*/, Interval /*score*/> tcp;
    std::map<Interval /*time*/, Interval /*score*/> http;
    std::map<Interval /*score*/, NQLevel> level;

    std::vector<double> weight{
        0.3, 0.7}; // the weight of each indicator. sum(tcp, http) = 1
    int nPower{2}; // weighting(nth power) to smooth data
  } _score;        // TODO: global config

  struct {
    std::mutex mutex;
    std::map<int64_t /*timestamp*/, const std::shared_ptr<NQIndicator>> map;
    int timeout{60 * 1000}; // default: 1 minutes // TODO: global config
  } _indicator;
};
