#pragma once
#include "IRedExtractor.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif
REDSOURCE_NS_BEGIN
class RedFFExtractor : public IRedExtractor {
public:
  RedFFExtractor();
  RedFFExtractor(const int &session_id, NotifyCallback notify_cb);
  int seek(int64_t timestamp, int64_t rel, int seek_flags) override;
  int open(const std::string &url, FFMpegOpt &opt,
           std::shared_ptr<MetaData> &metadata) override;
  int readPacket(AVPacket *pkt) override;
  void setInterrupt() override;
  int getPbError() override;
  int getStreamType(int stream_index) override;
  void close() override;
  ~RedFFExtractor();

private:
  static int interrupt_cb(void *opaque);
  void notifyListener(uint32_t what, int32_t arg1 = 0, int32_t arg2 = 0,
                      void *obj1 = nullptr, void *obj2 = nullptr,
                      int obj1_len = 0, int obj2_len = 0);

private:
  AVFormatContext *ic_{nullptr};
  std::atomic_bool interrupted_{false};
  const int session_id_{0};
  NotifyCallback notify_cb_;
};
REDSOURCE_NS_END
