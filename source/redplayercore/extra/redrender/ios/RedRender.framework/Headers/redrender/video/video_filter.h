/*
 * video_filter.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef VIDEO_FILTER_H
#define VIDEO_FILTER_H

#include "video_filter_info.h"
#include "video_inc_internal.h"

NS_REDRENDER_BEGIN

// filter manager base class
class VideoFilter {
public:
  VideoFilter(const int &sessionID = 0);
  virtual ~VideoFilter();

protected:
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* VIDEO_FILTER_H */
