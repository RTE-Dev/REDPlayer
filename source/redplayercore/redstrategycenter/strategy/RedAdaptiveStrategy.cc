#include "RedAdaptiveStrategy.h"
#include "RedLog.h"
#include "RedStrategyCenter.h"
#include "adaptive/logic/AdaptiveAdaptationLogic.h"
#include "adaptive/logic/AlwaysBestAdaptationLogic.h"
#include "adaptive/logic/AlwaysLowestAdaptationLogic.h"
#include "adaptive/playlist/RedPlaylistJsonParser.h"
#include "json.hpp"

namespace redstrategycenter {
namespace strategy {

RedAdaptiveStrategy::RedAdaptiveStrategy(
    adaptive::logic::AbstractAdaptationLogic::LogicType logicType) {
  network_evaluate_.reset(new evaluate::NetworkEvaluateV1(DEFAULT_MAX_WEIGHT));
  playlist_parser_.reset(new adaptive::playlist::RedPlaylistJsonParser());
  switch (logicType) {
  case adaptive::logic::AbstractAdaptationLogic::LogicType::AlwaysBest:
    adaptation_logic_.reset(new adaptive::logic::AlwaysBestAdaptationLogic());
    break;
  case adaptive::logic::AbstractAdaptationLogic::LogicType::AlwaysLowest:
    adaptation_logic_.reset(new adaptive::logic::AlwaysLowestAdaptationLogic());
    break;
  case adaptive::logic::AbstractAdaptationLogic::LogicType::Adaptive:
  default:
    adaptation_logic_.reset(new adaptive::logic::AdaptiveAdaptationLogic());
    break;
  }
}

RedAdaptiveStrategy::~RedAdaptiveStrategy() {}

int RedAdaptiveStrategy::setPlaylist(const std::string &str) {
  playlist_ = playlist_parser_->parse(str);
  return 0;
}

int RedAdaptiveStrategy::setReddownloadUrlCachedFunc(
    int64_t (*func)(const char *url)) {
  if (func != nullptr && adaptation_logic_ != nullptr) {
    return adaptation_logic_->setReddownloadUrlCachedFunc(func);
  }
  return -1;
}

int RedAdaptiveStrategy::getCurBandWidth() {
  int ret = -1;
  std::vector<Sample> samples = RedStrategyCenter::GetInstance()->getSamples();
  ret = network_evaluate_->getSpeed(
      adaptive::config::RedAdaptiveConfig::getInstance()
          ->getShortVideoConfigNePercentile(),
      samples);
  ret =
      static_cast<int>(ret * adaptive::config::RedAdaptiveConfig::getInstance()
                                 ->getShortVideoConfigNeScaleFactor());
  return ret;
}

int RedAdaptiveStrategy::getInitialRepresentation() {
  int ret = -1;
  if (playlist_ == nullptr || adaptation_logic_ == nullptr) {
    AV_LOGW(RED_STRATEGY_CENTER_TAG, "[%s:%d] failed\n", __FUNCTION__,
            __LINE__);
    return ret;
  }
  ret =
      adaptation_logic_->getInitialRepresentation(playlist_, getCurBandWidth());
  AV_LOGI(RED_STRATEGY_CENTER_TAG, "[%s:%d] getInitialRepresentation %d\n",
          __FUNCTION__, __LINE__, ret);
  return ret;
}

std::string RedAdaptiveStrategy::getInitialUrl(int index) {
  std::string ret;
  index = index < 0 ? 0 : index;
  if (index < playlist_->adaptation_set->representations.size()) {
    ret = playlist_->adaptation_set->representations[index]->url;
    AV_LOGI(RED_STRATEGY_CENTER_TAG, "[%s:%d] getInitialUrl %s %dx%d\n",
            __FUNCTION__, __LINE__, ret.c_str(),
            playlist_->adaptation_set->representations[index]->width,
            playlist_->adaptation_set->representations[index]->height);
  }
  return ret;
}

std::string RedAdaptiveStrategy::getInitialUrlList(int index) {
  std::string ret;
  index = index < 0 ? 0 : index;
  if (index < playlist_->adaptation_set->representations.size()) {
    ret = playlist_->adaptation_set->representations[index]->url;
    for (int i = 0;
         i <
         playlist_->adaptation_set->representations[index]->backup_urls.size();
         i++) {
      ret += ";" +
             playlist_->adaptation_set->representations[index]->backup_urls[i];
    }
    AV_LOGI(RED_STRATEGY_CENTER_TAG, "[%s:%d] getInitialUrlList %s %dx%d\n",
            __FUNCTION__, __LINE__, ret.c_str(),
            playlist_->adaptation_set->representations[index]->width,
            playlist_->adaptation_set->representations[index]->height);
  }
  return ret;
}

} // namespace strategy
} // namespace redstrategycenter
