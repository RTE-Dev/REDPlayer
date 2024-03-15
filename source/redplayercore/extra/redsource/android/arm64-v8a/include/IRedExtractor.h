#pragma once
#include <algorithm>

#include "RedSourceCommon.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif

namespace redsource {
class IRedExtractor {
public:
  virtual int seek(int64_t timestamp, int64_t rel = 0, int seek_flags = 0) = 0;
  virtual int open(const std::string &url, FFMpegOpt &opt,
                   std::shared_ptr<MetaData> &metadata) = 0;
  virtual int readPacket(AVPacket *pkt) = 0;
  virtual void setInterrupt() = 0;
  virtual int getPbError() = 0;
  virtual int getStreamType(int stream_index) = 0;
  virtual void close() = 0;
  virtual ~IRedExtractor() = default;
};
} // namespace redsource