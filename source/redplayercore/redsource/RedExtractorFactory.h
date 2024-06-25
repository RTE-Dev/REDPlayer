#pragma once
#include "IRedExtractor.h"
REDSOURCE_NS_BEGIN
class RedExtractorFactory {
public:
  static std::unique_ptr<IRedExtractor>
  create(const int &session_id = 0, ExtractorType type = ExtractorType::FFMpeg,
         NotifyCallback notify_cb = nullptr);
};
REDSOURCE_NS_END
