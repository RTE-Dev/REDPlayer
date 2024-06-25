#pragma once

#include "adaptive/logic/AbstractAdaptationLogic.h"

namespace redstrategycenter {
namespace adaptive {
namespace logic {
class AlwaysLowestAdaptationLogic : public AbstractAdaptationLogic {
public:
  AlwaysLowestAdaptationLogic() = default;
  ~AlwaysLowestAdaptationLogic() = default;
  int getInitialRepresentation(const std::unique_ptr<playlist::PlayList> &p,
                               const int speed) override;
};
} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
