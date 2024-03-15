#pragma once

#include "RedAdaptiveConfig.h"
#include "json.hpp"
#include <string>
#include <vector>

namespace redstrategycenter {
namespace adaptive {
namespace config {
class RedAdaptiveConfigParser {
public:
  RedAdaptiveConfigParser() = default;

  ~RedAdaptiveConfigParser() = default;

  ShortVideoConfig *parseShortVideoConfig(const std::string &str);
};
} // namespace config
} // namespace adaptive
} // namespace redstrategycenter