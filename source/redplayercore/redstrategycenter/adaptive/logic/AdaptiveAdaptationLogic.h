#pragma once

#include "adaptive/logic/AbstractAdaptationLogic.h"

namespace redstrategycenter {
namespace adaptive {
namespace logic {
class AdaptiveAdaptationLogic : public AbstractAdaptationLogic {
public:
  AdaptiveAdaptationLogic() = default;
  ~AdaptiveAdaptationLogic() = default;
  int getInitialRepresentation(const std::unique_ptr<playlist::PlayList> &p,
                               const int speed) override;
  int setReddownloadUrlCachedFunc(int64_t (*func)(const char *url)) override;
};
} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
