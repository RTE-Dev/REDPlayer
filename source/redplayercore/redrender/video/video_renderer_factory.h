#pragma once

#include <memory>
#include <vector>

#include "./video_filter.h"
#include "./video_inc_internal.h"
#include "./video_renderer.h"
#include "./video_renderer_info.h"

NS_REDRENDER_BEGIN

class VideoRendererFactory {
public:
  VideoRendererFactory();
  ~VideoRendererFactory();

  std::unique_ptr<VideoRenderer>
  createVideoRenderer(const VideoRendererInfo &videoRendererInfo,
                      const int &sessionID = 0);
  bool isSupportedRender(const VideoRendererInfo &videoRendererInfo);

private:
  std::vector<VideoRendererInfo> _supportedRenderer;
};

NS_REDRENDER_END
