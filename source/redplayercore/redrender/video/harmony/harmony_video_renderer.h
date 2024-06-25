#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
#pragma once

#include <memory>

#include "../video_filter.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

NS_REDRENDER_BEGIN

class HarmonyVideoRenderer : public VideoRenderer {
public:
  HarmonyVideoRenderer(const int &sessionID);
  ~HarmonyVideoRenderer() override;
  VRError onRender(VideoHarmonyDecoderBufferContext *odBufferCtx,
                   bool render) override;
};

NS_REDRENDER_END
#endif