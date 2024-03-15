#pragma once
#include "IRedExtractor.h"
#include "RedDef.h"
#include "RedSourceCommon.h"
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>
namespace redsource {
class RedSource {
public:
  RedSource();
  RedSource(const int &session_id, NotifyCallback notify_cb);
  ~RedSource();
  RedSource(const RedSource &) = delete;
  RedSource &operator=(const RedSource &) = delete;
  RedSource(RedSource &&) = delete;
  RedSource &operator=(RedSource &&) = delete;
  int seek(int64_t timestamp, int64_t rel = 0, int seek_flags = 0);
  int open(const std::string &url, FFMpegOpt &opt,
           std::shared_ptr<MetaData> &metadata);
  int readPacket(AVPacket *pkt);
  void setInterrupt();
  int getPbError();
  int getStreamType(int stream_index);
  void close();

private:
  const int session_id_{0};
  std::unique_ptr<IRedExtractor> extractor_{nullptr};
};
} // namespace redsource
