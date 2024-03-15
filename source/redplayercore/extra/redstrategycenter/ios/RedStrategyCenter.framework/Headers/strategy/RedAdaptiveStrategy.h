#pragma once

#include "RedStrategyCenterCommon.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include "adaptive/logic/AbstractAdaptationLogic.h"
#include "adaptive/logic/DefaultAdaptationLogic.h"
#include "adaptive/playlist/RedPlaylistParser.h"
#include "evaluate/NetworkEvaluate.h"
#include "evaluate/NetworkEvaluateV1.h"

using redstrategycenter::adaptive::logic::AbstractAdaptationLogic;
using redstrategycenter::evaluate::NetworkEvaluate;
namespace redstrategycenter {
namespace strategy {
class RedAdaptiveStrategy {
public:
  RedAdaptiveStrategy(AbstractAdaptationLogic::LogicType);
  ~RedAdaptiveStrategy();
  int setPlaylist(const std::string &str);
  int setReddownloadUrlCachedFunc(int64_t (*func)(const char *url));
  int setCacheDir(const char *cache_dir);
  int getInitialRepresentation();
  std::string getInitialUrl(int index);
  std::string getInitialUrlList(int index);
  int getCurBandWidth();

private:
  int stream_type_;
  int band_width_;
  std::string quality_type_;
  playListParser *playlist_parser_;
  PlayList *playlist_;
  NetworkEvaluate *network_evaluate_;
  AbstractAdaptationLogic *adaptation_logic_;
};
} // namespace strategy
} // namespace redstrategycenter
