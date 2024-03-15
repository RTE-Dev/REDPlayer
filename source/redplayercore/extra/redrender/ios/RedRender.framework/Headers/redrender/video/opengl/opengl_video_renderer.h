/*
 * opengl_video_renderer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef OPENGL_VIDEO_RENDERER_H
#define OPENGL_VIDEO_RENDERER_H

#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "../video_filter.h"
#include "../video_filter_info.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"
#include "common/context.h"
#include "common/framebuffer.h"
#include "filter/opengl_filter_base.h"
#include <sys/time.h>
#include <time.h>

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "redrender_gl_view.h"
#include <mach/mach_time.h>
#endif

NS_REDRENDER_BEGIN

typedef enum filterPriorityType {
  // todo
} filterPriorityType;

class OpenGLVideoRenderer : public VideoRenderer {
public:
  OpenGLVideoRenderer(
      const VideoRendererInfo &videoRendererInfo = defaultVideoRendererInfo,
      const VideoFilterInfo &videoFilterInfo = defaultVideoFilterInfo,
      const int &sessionID = 0);
  virtual ~OpenGLVideoRenderer() override;
  virtual VRError releaseContext() override;
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  virtual VRError init(ANativeWindow *nativeWindow = nullptr) override;
  virtual VRError setSurface(ANativeWindow *nativeWindow = nullptr) override;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  virtual VRError init() override;
  virtual VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
  virtual UIView *getRedRenderView() override;
#endif
  // filter chain management
  virtual VRError attachFilter(VideoFilterType videoFilterType,
                               VideoFrameMetaData *inputFrameMetaData = nullptr,
                               int priority = -1) override;
  virtual VRError detachFilter(VideoFilterType videoFilterType) override;
  virtual VRError detachAllFilter() override;

  VRError updateFilterPriority(VideoFilterType videoFilterType,
                               int priority); // todo
  // update param
  VRError onUpdateParam(VideoFilterType videoFilterType) override;
  // input Frame
  VRError onInputFrame(VideoFrameMetaData *redRenderBuffer) override;
  // render
  VRError onRender() override;

  VRError onRenderCachedFrame() override;

  static VideoRendererInfo defaultVideoRendererInfo;
  static VideoFilterInfo defaultVideoFilterInfo;

  VRError setGravity(RedRender::GravityResizeAspectRatio rendererGravity);

protected:
  std::map<int /* priority */, std::shared_ptr<OpenGLFilterBase>>
      _openglFilterChain; /* Store post-processing filters */
  std::shared_ptr<OpenGLFilterBase> _openglFilterDevice =
      nullptr; /* Store on-screen filter */
  std::queue<std::shared_ptr<Framebuffer>>
      _textureBufferQueue; /* The output texture is processed after storage */
private:
  VRError _setInputFrame(VideoFrameMetaData *inputFrameMetaData);
  VRError _post_processing();
  VRError _on_screen_render();
  VRError _createOnScreenRender(VideoFrameMetaData *inputFrameMetaData);
  VRError _attachFilteruUpdateTarget(int curFilterPriority);
  VRError _detachFilteruUpdateTarget(int curFilterPriority);
  void _clearTextureBufferQueue();
  void _updateInputFrameMetaData(VideoFrameMetaData *inputFrameMetaData);

  std::shared_ptr<RedRender::Context> mInstance;

  std::shared_ptr<Framebuffer> _srcPlaneTextures[3] = {nullptr};
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  CVPixelBufferRef mCachedPixelBuffer{nullptr};
#endif
  RedRender::GravityResizeAspectRatio mRendererGravity{
      RedRender::ScaleAspectFit};

  std::mutex _rendererMutex;
  VideoFrameMetaData _onScreenMetaData;
  VideoFrameMetaData *_inputFrameMetaData{nullptr};
  int _layerWidth;
  int _layerHeight;

  // fps
  double _fps;
  uint64_t _lastFrameTime = 0;
  int _frameCount = 0;
  uint64_t _getClock();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  static int g_is_mach_base_info_inited;
  static kern_return_t g_mach_base_info_ret;
  static mach_timebase_info_data_t g_mach_base_info;
  RedRenderGLView *mRedRenderGLView{nullptr};
  RedRenderGLTexture *mRedRenderGLTexture[2] = {nullptr};
#endif
};

NS_REDRENDER_END

#endif /* OPENGL_VIDEO_RENDERER_H */
