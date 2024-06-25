#include "RedExtractorFactory.h"
#include "RedFFExtractor.h"
REDSOURCE_NS_BEGIN

std::unique_ptr<IRedExtractor>
RedExtractorFactory::create(const int &session_id, ExtractorType type,
                            NotifyCallback notify_cb) {
  switch (type) {
  case ExtractorType::FFMpeg: {
    RedFFExtractor *extractor = new RedFFExtractor(session_id, notify_cb);
    return std::unique_ptr<IRedExtractor>(extractor);
  }
  default:
    return nullptr;
  }
}
REDSOURCE_NS_END
