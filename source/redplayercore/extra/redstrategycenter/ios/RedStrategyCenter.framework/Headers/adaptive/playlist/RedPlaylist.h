#pragma once

#include "RedStrategyCenterCommon.h"
#include <string>
#include <vector>
namespace redstrategycenter {
namespace playlist {
typedef struct Representation {
  std::string url;
  std::string quality_type;
  int stream_type;
  int avg_bitrate;
  int width;
  int height;
  int weight;
  bool is_default;
  std::vector<std::string> backup_urls;
} Representation;

typedef struct AdaptationSet {
  int32_t duration;
  std::vector<Representation *> representations;
} AdaptationSet;

typedef struct PlayList {
  AdaptationSet *adaptation_set;
} PlayList;

inline void releasePlaylist(PlayList **playlist) {
  if (playlist != nullptr && (*playlist) != nullptr) {
    if ((*playlist)->adaptation_set != nullptr) {
      for (int i = 0; i < (*playlist)->adaptation_set->representations.size();
           i++) {
        delete (*playlist)->adaptation_set->representations[i];
      }
      (*playlist)->adaptation_set->representations.clear();
      delete (*playlist)->adaptation_set;
      (*playlist)->adaptation_set = nullptr;
    }
    delete (*playlist);
    (*playlist) = nullptr;
  }
}
} // namespace playlist
} // namespace redstrategycenter