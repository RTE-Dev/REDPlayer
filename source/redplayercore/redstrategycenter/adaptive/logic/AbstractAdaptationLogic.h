#pragma once

#include "adaptive/playlist/RedPlaylist.h"
#include "adaptive/playlist/RedPlaylistParser.h"

namespace redstrategycenter {
namespace adaptive {
namespace logic {
class AbstractAdaptationLogic {

public:
  enum class LogicType {
    Adaptive = 0,
    AlwaysBest = 1,
    AlwaysLowest = 2,
  };
  AbstractAdaptationLogic() = default;
  virtual ~AbstractAdaptationLogic() = default;
  virtual int
  getInitialRepresentation(const std::unique_ptr<playlist::PlayList> &p,
                           const int speed) = 0;
  virtual int setReddownloadUrlCachedFunc(int64_t (*func)(const char *url)) {
    return 0;
  }

protected:
  int64_t (*reddownload_func_)(const char *url){nullptr};
};
} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
