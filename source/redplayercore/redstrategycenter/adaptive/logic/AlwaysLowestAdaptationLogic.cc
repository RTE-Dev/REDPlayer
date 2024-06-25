#include "adaptive/logic/AlwaysLowestAdaptationLogic.h"
#include "RedLog.h"
#include "RedStrategyCenterCommon.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include <float.h>
#include <inttypes.h>
#include <string>

namespace redstrategycenter {
namespace adaptive {
namespace logic {

int AlwaysLowestAdaptationLogic::getInitialRepresentation(
    const std::unique_ptr<playlist::PlayList> &p, const int speed) {
  return static_cast<int>(p->adaptation_set->representations.size()) - 1;
}

} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
