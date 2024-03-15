#pragma once

#include "AbstractAdaptationLogic.h"

namespace redstrategycenter {
namespace adaptive {
namespace logic {
class DefaultAdaptationLogic : public AbstractAdaptationLogic {
public:
  DefaultAdaptationLogic() = default;
  ~DefaultAdaptationLogic() = default;
  int getInitialRepresentation(const PlayList *p, const int speed) override;
  int setReddownloadUrlCachedFunc(int64_t (*func)(const char *url)) override;
  int setCacheDir(const char *cache_dir) override;

private:
  static bool Comparator(const Representation *a, const Representation *b);
};
} // namespace logic
} // namespace adaptive
} // namespace redstrategycenter