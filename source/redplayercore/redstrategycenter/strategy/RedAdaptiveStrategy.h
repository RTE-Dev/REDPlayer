#pragma once

#include "RedStrategyCenterCommon.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include "adaptive/logic/AbstractAdaptationLogic.h"
#include "adaptive/playlist/RedPlaylistParser.h"
#include "evaluate/NetworkEvaluate.h"
#include "evaluate/NetworkEvaluateV1.h"

namespace redstrategycenter {
namespace strategy {
class RedAdaptiveStrategy {
public:
  RedAdaptiveStrategy(adaptive::logic::AbstractAdaptationLogic::LogicType);
  ~RedAdaptiveStrategy();
  int setPlaylist(const std::string &str);
  int setReddownloadUrlCachedFunc(int64_t (*func)(const char *url));
  int getInitialRepresentation();
  std::string getInitialUrl(int index);
  std::string getInitialUrlList(int index);
  int getCurBandWidth();
  RedAdaptiveStrategy(const RedAdaptiveStrategy &) = delete;
  RedAdaptiveStrategy &operator=(const RedAdaptiveStrategy &) = delete;
  RedAdaptiveStrategy(RedAdaptiveStrategy &&) = delete;
  RedAdaptiveStrategy &operator=(RedAdaptiveStrategy &&) = delete;

private:
  std::unique_ptr<adaptive::playlist::playListParser> playlist_parser_;
  std::unique_ptr<adaptive::playlist::PlayList> playlist_;
  std::unique_ptr<evaluate::NetworkEvaluate> network_evaluate_;
  std::unique_ptr<adaptive::logic::AbstractAdaptationLogic> adaptation_logic_;
};
} // namespace strategy
} // namespace redstrategycenter
