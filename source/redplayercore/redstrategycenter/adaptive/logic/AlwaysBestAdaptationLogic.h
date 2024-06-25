#pragma once

#include "adaptive/logic/AbstractAdaptationLogic.h"

namespace redstrategycenter {
namespace adaptive {
namespace logic {
class AlwaysBestAdaptationLogic : public AbstractAdaptationLogic {
public:
  AlwaysBestAdaptationLogic() = default;
  ~AlwaysBestAdaptationLogic() = default;
  int getInitialRepresentation(const std::unique_ptr<playlist::PlayList> &p,
                               const int speed) override;
};
} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
