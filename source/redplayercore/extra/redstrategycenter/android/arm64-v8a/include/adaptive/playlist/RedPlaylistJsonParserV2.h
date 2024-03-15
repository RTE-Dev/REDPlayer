#pragma once

#include "RedPlaylistParser.h"
#include <string>
#include <vector>
namespace redstrategycenter {
namespace playlist {
class RedPlaylistJsonParserV2 : public playListParser {
public:
  RedPlaylistJsonParserV2();

  ~RedPlaylistJsonParserV2() override;

  PlayList *parse(const std::string &str) override;

private:
  std::vector<std::string> getCodecNameList();
};
} // namespace playlist
} // namespace redstrategycenter