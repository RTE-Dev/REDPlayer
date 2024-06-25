#pragma once

#include "adaptive/playlist/RedPlaylist.h"
#include <map>
#include <string>
#include <vector>
namespace redstrategycenter {
namespace adaptive {
namespace playlist {
class playListParser {
public:
  playListParser() = default;

  virtual ~playListParser() = default;

  virtual std::unique_ptr<PlayList> parse(const std::string &str) = 0;
};
} // namespace playlist
} // namespace adaptive
} // namespace redstrategycenter
