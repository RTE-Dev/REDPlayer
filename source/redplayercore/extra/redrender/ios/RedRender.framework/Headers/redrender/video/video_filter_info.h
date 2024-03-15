/*
 * video_filter_info.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef VIDEO_FILTER_INFO_H
#define VIDEO_FILTER_INFO_H

#include "video_inc_internal.h"

NS_REDRENDER_BEGIN

typedef enum VideoFilterClusterType {
  VideoFilterClusterTypeUnknown = 0,
  VideoFilterClusterTypeOpenGL = 1,
  VideoFilterClusterTypeMetal = 2,
  // todo : add Vulkan, DX11
} VideoFilterClusterType;

class VideoFilterInfo {
public:
  VideoFilterInfo(
      VideoFilterClusterType filterType = VideoFilterClusterTypeUnknown);
  ~VideoFilterInfo() = default;

  VideoFilterClusterType _videoFilterClusterType;
};

NS_REDRENDER_END

#endif /* VIDEO_FILTER_INFO_H */
