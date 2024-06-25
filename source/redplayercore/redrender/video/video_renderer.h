#pragma once

#ifdef __HARMONY__
#include <multimedia/player_framework/native_avcodec_base.h>
#endif

#include "../common/redrender_buffer.h"
#include "./video_filter.h"
#include "./video_inc_internal.h"
#include "./video_renderer_info.h"
#include "opengl/filter/opengl_filter_base.h"
#include <memory>

NS_REDRENDER_BEGIN

// renderer base class
class VideoRenderer {
public:
  explicit VideoRenderer(const int &sessionID);
  virtual ~VideoRenderer();
  // OpenGL interface
  virtual VRError releaseContext();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  virtual VRError init(ANativeWindow *nativeWindow = nullptr);
  virtual VRError setSurface(ANativeWindow *nativeWindow = nullptr);
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
  virtual VRError init(OHNativeWindow *nativeWindow = nullptr);
  virtual VRError setSurface(OHNativeWindow *nativeWindow = nullptr);
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  virtual VRError init();
  virtual VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}});
  virtual UIView *getRedRenderView();
#endif
  virtual VRError attachFilter(VideoFilterType videoFilterType,
                               VideoFrameMetaData *inputFrameMetaData = nullptr,
                               int priority = -1);
  virtual VRError detachFilter(VideoFilterType videoFilterType);
  virtual VRError detachAllFilter();
  virtual VRError onUpdateParam(VideoFilterType videoFilterType);
  virtual VRError onInputFrame(VideoFrameMetaData *redRenderBuffer);
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  // MediaCodec interface
  virtual VRError onRender(VideoMediaCodecBufferContext *odBufferCtx,
                           bool render);
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
  // HarmonyDecoder interface
  virtual VRError onRender(VideoHarmonyDecoderBufferContext *odBufferCtx,
                           bool render);
#endif
  // Common interface
  virtual VRError onRender();
  virtual VRError onRenderCachedFrame();
  virtual void close(){};

protected:
  const int _sessionID{0};
};

NS_REDRENDER_END
