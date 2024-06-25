#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#pragma once

#include <memory>

#include "../video_filter.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

NS_REDRENDER_BEGIN

class MediaCodecVideoRenderer : public VideoRenderer {
public:
  MediaCodecVideoRenderer(const int &sessionID);
  ~MediaCodecVideoRenderer();

  VRError onRender(VideoMediaCodecBufferContext *odBufferCtx,
                   bool render) override;
};

NS_REDRENDER_END
#endif