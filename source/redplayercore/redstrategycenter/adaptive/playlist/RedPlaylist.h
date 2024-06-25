#pragma once

#include "RedStrategyCenterCommon.h"
#include <string>
#include <vector>
namespace redstrategycenter {
namespace adaptive {
namespace playlist {
typedef struct Representation {
  std::string url;
  int avg_bitrate;
  int width;
  int height;
  int weight;
  std::vector<std::string> backup_urls;
} Representation;

static inline bool Comparator(const std::unique_ptr<Representation> &a,
                              const std::unique_ptr<Representation> &b) {
  if (a->weight < b->weight && a->weight >= 0) {
    return false;
  } else if (a->weight > b->weight && b->weight >= 0) {
    return true;
  } else {
    return ((a->width * a->height) > (b->width * b->height));
  }
}

typedef struct AdaptationSet {
  int32_t duration;
  std::vector<std::unique_ptr<Representation>> representations;
} AdaptationSet;

typedef struct PlayList {
  std::unique_ptr<AdaptationSet> adaptation_set;
} PlayList;

} // namespace playlist
} // namespace adaptive
} // namespace redstrategycenter
