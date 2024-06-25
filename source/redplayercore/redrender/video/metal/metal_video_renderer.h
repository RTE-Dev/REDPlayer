#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include <mach/mach_time.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sys/time.h>
#include <time.h>

#include "../video_filter.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

#import "metal_filter_base.h"
#import "redrender_metal_view.h"
#import <MetalKit/MetalKit.h>

NS_REDRENDER_BEGIN

class MetalVideoRenderer : public VideoRenderer {
public:
  MetalVideoRenderer(const int &sessionID);
  ~MetalVideoRenderer();
  VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
  UIView *getRedRenderView() override;

  VRError attachFilter(VideoFilterType videoFilterType,
                       VideoFrameMetaData *inputFrameMetaData = nullptr,
                       int priority = -1) override;
  VRError detachFilter(VideoFilterType videoFilterType) override;
  VRError detachAllFilter() override;

  // input Frame
  VRError onInputFrame(VideoFrameMetaData *redRenderBuffer) override;
  // render
  VRError onRender() override;

  VRError onRenderCachedFrame() override;

  void close() override;

  VRError setGravity(RedRender::GravityResizeAspectRatio rendererGravity);

protected:
  RedRenderMetalView *mMetalView{nullptr};

protected:
  MetalFilterBase *_metalFilterDevice = nullptr; /* Store on-screen filter */
  std::queue<MetalVideoTexture *>
      _textureBufferQueue; /* The output texture is processed after storage */

private:
  VRError _setInputFrame(VideoFrameMetaData *inputFrameMetaData);
  VRError _on_screen_render();
  VRError _createOnScreenRender(VideoFrameMetaData *inputFrameMetaData);
  void _clearTextureBufferQueue();
  void _updateInputFrameMetaData(VideoFrameMetaData *inputFrameMetaData);

  MetalVideoCmdQueue *mMetalVideoCmdQueue{nullptr};

  MetalVideoTexture *_srcPlaneTextures[3] = {nullptr};
  CVPixelBufferRef mCachedPixelBuffer{nullptr};
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
};

NS_REDRENDER_END

#endif
