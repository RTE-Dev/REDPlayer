#pragma once
#include "IRedExtractor.h"
#include "RedSourceCommon.h"
namespace redsource {
class RedExtractorFactory {
public:
  static std::unique_ptr<IRedExtractor>
  create(const int &session_id = 0, ExtractorType type = ExtractorType::FFMpeg,
         NotifyCallback notify_cb = nullptr);
};
} // namespace redsource
