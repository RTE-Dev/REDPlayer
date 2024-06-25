#include "adaptive/logic/AlwaysBestAdaptationLogic.h"
#include "RedLog.h"
#include "RedStrategyCenterCommon.h"
#include "adaptive/config/RedAdaptiveConfig.h"
#include <float.h>
#include <inttypes.h>
#include <string>

namespace redstrategycenter {
namespace adaptive {
namespace logic {

int AlwaysBestAdaptationLogic::getInitialRepresentation(
    const std::unique_ptr<playlist::PlayList> &p, const int speed) {
  return 0;
}

} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter
