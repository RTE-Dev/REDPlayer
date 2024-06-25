#include "adaptive/playlist/RedPlaylistJsonParser.h"
#include "RedLog.h"
#include "RedStrategyCenterCommon.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include "json.hpp"
#include <iostream>

namespace redstrategycenter {
namespace adaptive {
namespace playlist {

std::vector<std::string> RedPlaylistJsonParser::getCodecNameList() {
  return {"av1", "h265", "h264"};
}

std::unique_ptr<PlayList> RedPlaylistJsonParser::parse(const std::string &str) {
  std::unique_ptr<PlayList> ret = std::unique_ptr<PlayList>(new PlayList());
  ret->adaptation_set.reset(new AdaptationSet());
  try {
    nlohmann::json j = nlohmann::json::parse(str);

    const std::vector<std::string> red_codec_name_list = getCodecNameList();
    for (int codec_index = 0; codec_index < red_codec_name_list.size();
         codec_index++) {
      bool found_codec = 0;
      std::string red_codec_name = red_codec_name_list[codec_index];
      for (int i = 0; i < j["stream"][red_codec_name].size(); i++) {
        int width =
            static_cast<int>(j["stream"][red_codec_name][i].value("width", 0));
        int height =
            static_cast<int>(j["stream"][red_codec_name][i].value("height", 0));
        std::unique_ptr<Representation> representation =
            std::unique_ptr<Representation>(new Representation());
        representation->url =
            (std::string)j["stream"][red_codec_name][i].value("master_url", "");
        if (!representation->url.empty()) {
          representation->avg_bitrate = static_cast<int>(
              j["stream"][red_codec_name][i].value("avg_bitrate", __INT_MAX__));
          representation->width = width;
          representation->height = height;
          representation->weight = static_cast<int>(
              j["stream"][red_codec_name][i].value("weight", -1));
          for (int k = 0;
               k < j["stream"][red_codec_name][i]["backup_urls"].size(); k++) {
            std::string backup_url = static_cast<std::string>(
                j["stream"][red_codec_name][i]["backup_urls"][k]);
            representation->backup_urls.emplace_back(backup_url);
          }
          ret->adaptation_set->representations.push_back(
              std::move(representation));
          if (!found_codec) {
            found_codec = true;
          }
        }
      }

      if (found_codec) {
        std::sort(ret->adaptation_set->representations.begin(),
                  ret->adaptation_set->representations.end(), Comparator);
      }
    }
    return ret;
  } catch (nlohmann::json::exception &e) {
    AV_LOGE(RED_STRATEGY_CENTER_TAG, "[%s:%d] error: %s\n", __FUNCTION__,
            __LINE__, e.what());
    return ret;
  } catch (...) {
    AV_LOGE(RED_STRATEGY_CENTER_TAG, "[%s:%d] error\n", __FUNCTION__, __LINE__);
    return ret;
  }
}

} // namespace playlist
} // namespace adaptive
} // namespace redstrategycenter
