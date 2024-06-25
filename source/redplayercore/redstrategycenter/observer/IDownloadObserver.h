#pragma once

#include <cstdint>

namespace redstrategycenter {
namespace observer {
class IDownloadObserver {
public:
  virtual void updateDownloadRate(int64_t size, int64_t speed,
                                  int64_t cur_time) = 0;
  virtual ~IDownloadObserver() {}
};
} // namespace observer
} // namespace redstrategycenter
