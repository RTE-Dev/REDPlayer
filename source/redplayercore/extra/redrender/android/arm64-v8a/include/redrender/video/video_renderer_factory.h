/*
 * video_renderer_factory.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu
 *
 * This file is part of RedRender.
 */

#ifndef VIDEO_RENDERER_FACTORY_H
#define VIDEO_RENDERER_FACTORY_H

#include "video_filter.h"
#include "video_filter_info.h"
#include "video_inc_internal.h"
#include "video_renderer.h"
#include "video_renderer_info.h"
#include <memory>
#include <vector>

NS_REDRENDER_BEGIN

class VideoRendererFactory {

public:
  VideoRendererFactory();
  ~VideoRendererFactory();

  std::unique_ptr<VideoRenderer>
  createVideoRenderer(const VideoRendererInfo &videoRendererInfo,
                      const VideoFilterInfo &videoFilterInfo,
                      const int &sessionID = 0);
  bool isSupportedRender(const VideoRendererInfo &videoRendererInfo,
                         const VideoFilterInfo &videoFilterInfo);

private:
  std::vector<VideoRendererInfo> _supportedRenderer;
  std::vector<VideoFilterInfo> _supportedFilter;
};

NS_REDRENDER_END

#endif /* VIDEO_RENDERER_FACTORY_H */
