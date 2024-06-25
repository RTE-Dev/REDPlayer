#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "../video_filter.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"
#include "common/context.h"
#include "common/framebuffer.h"
#include "filter/opengl_filter_base.h"
#include <sys/time.h>
#include <time.h>

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "./ios/redrender_gl_view.h"
#include <mach/mach_time.h>
#endif

NS_REDRENDER_BEGIN

typedef enum filterPriorityType {
  // todo
} filterPriorityType;

class OpenGLVideoRenderer : public VideoRenderer {
public:
  OpenGLVideoRenderer(const int &sessionID);
  ~OpenGLVideoRenderer();
  VRError releaseContext() override;
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  VRError init(ANativeWindow *nativeWindow = nullptr) override;
  VRError setSurface(ANativeWindow *nativeWindow = nullptr) override;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
  VRError init(OHNativeWindow *nativeWindow = nullptr) override;
  VRError setSurface(OHNativeWindow *nativeWindow = nullptr) override;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  VRError init() override;
  VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
  UIView *getRedRenderView() override;
#endif
  // filter chain management
  VRError attachFilter(VideoFilterType videoFilterType,
                       VideoFrameMetaData *inputFrameMetaData = nullptr,
                       int priority = -1) override;

  // input Frame
  VRError onInputFrame(VideoFrameMetaData *redRenderBuffer) override;
  // render
  VRError onRender() override;

  VRError onRenderCachedFrame() override;

  VRError setGravity(RedRender::GravityResizeAspectRatio rendererGravity);

protected:
  std::shared_ptr<OpenGLFilterBase> _openglFilterDevice =
      nullptr; /* Store on-screen filter */
  std::queue<std::shared_ptr<Framebuffer>>
      _textureBufferQueue; /* The output texture is processed after storage */

private:
  VRError _setInputFrame(VideoFrameMetaData *inputFrameMetaData);
  VRError _on_screen_render();
  VRError _createOnScreenRender(VideoFrameMetaData *inputFrameMetaData);
  void _updateInputFrameMetaData(VideoFrameMetaData *inputFrameMetaData);

  std::shared_ptr<RedRender::Context> mInstance;

  std::shared_ptr<Framebuffer> _srcPlaneTextures[3] = {nullptr};
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  CVPixelBufferRef mCachedPixelBuffer{nullptr};
#endif
  RedRender::GravityResizeAspectRatio mRendererGravity{
      RedRender::ScaleAspectFit};

  std::mutex _rendererMutex;
  VideoFrameMetaData _onScreenMetaData {
    {nullptr, nullptr, nullptr}, {0, 0, 0}, 0,
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
        nullptr,
#endif
        0, 0, 0, 0, VRPixelFormatUnknown, VR_AVCOL_PRI_RESERVED0,
        VR_AVCOL_TRC_RESERVED0, VR_AVCOL_SPC_RGB, VR_AVCOL_RANGE_UNSPECIFIED,
        AspectRatioEqualWidth, NoRotation, {
      0, 0
    }
  };

  VideoFrameMetaData *_inputFrameMetaData{nullptr};
  int _layerWidth{0};
  int _layerHeight{0};

  // fps
  double _fps{0};
  uint64_t _lastFrameTime = 0;
  int _frameCount = 0;
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  RedRenderGLView *mRedRenderGLView{nullptr};
  RedRenderGLTexture *mRedRenderGLTexture[2] = {nullptr};
#endif
};

NS_REDRENDER_END
