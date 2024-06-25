#pragma once

#include "adaptive/playlist/RedPlaylistParser.h"
#include <string>
#include <vector>
namespace redstrategycenter {
namespace adaptive {
namespace playlist {
class RedPlaylistJsonParser : public playListParser {
public:
  RedPlaylistJsonParser() = default;

  ~RedPlaylistJsonParser() = default;

  std::unique_ptr<PlayList> parse(const std::string &str) override;

private:
  std::vector<std::string> getCodecNameList();
};
} // namespace playlist
} // namespace adaptive
} // namespace redstrategycenter
