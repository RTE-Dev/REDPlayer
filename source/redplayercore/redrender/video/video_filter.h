#pragma once

#include "./video_inc_internal.h"

NS_REDRENDER_BEGIN

// filter manager base class
class VideoFilter {
public:
  VideoFilter(const int &sessionID = 0);
  virtual ~VideoFilter();

private:
  const int _sessionID{0};
};

NS_REDRENDER_END
