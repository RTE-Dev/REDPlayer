#pragma once

#include "adaptive/playlist/RedPlaylist.h"
#include "adaptive/playlist/RedPlaylistParser.h"

using redstrategycenter::playlist::PlayList;
using redstrategycenter::playlist::playListParser;
using redstrategycenter::playlist::Representation;
namespace redstrategycenter {
namespace adaptive {
namespace logic {
class AbstractAdaptationLogic {

public:
  enum class LogicType {
    Adaptive = 0,
    // The current version does not include
    AlwaysBest = 1,
    AlwaysLowest = 2,
    AlwaysDefault = 3,
  };
  AbstractAdaptationLogic() = default;
  virtual ~AbstractAdaptationLogic() = default;
  virtual int getInitialRepresentation(const PlayList *p, const int speed) = 0;
  virtual int setReddownloadUrlCachedFunc(int64_t (*func)(const char *url)) = 0;
  virtual int setCacheDir(const char *cache_dir) = 0;

protected:
  int64_t (*reddownload_func_)(const char *url){nullptr};
  std::string cache_dir_;
  bool select_cached_url_{false};
  int64_t url_cached_size_{0};
};
} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter