/*
 * metal_video_renderer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef METAL_VIDEO_RENDERER_H
#define METAL_VIDEO_RENDERER_H

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include <mach/mach_time.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sys/time.h>
#include <time.h>

#include "../video_filter.h"
#include "../video_filter_info.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

#import "metal_filter_base.h"
#import "redrender_metal_view.h"
#import <MetalKit/MetalKit.h>

NS_REDRENDER_BEGIN

class MetalVideoRenderer : public VideoRenderer {
public:
  MetalVideoRenderer(
      const VideoRendererInfo &videoRendererInfo = defaultVideoRendererInfo,
      const VideoFilterInfo &videoFilterInfo = defaultVideoFilterInfo,
      const int &sessionID = 0);
  virtual ~MetalVideoRenderer() override;
  virtual VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
  virtual UIView *getRedRenderView() override;

  virtual VRError attachFilter(VideoFilterType videoFilterType,
                               VideoFrameMetaData *inputFrameMetaData = nullptr,
                               int priority = -1) override;
  virtual VRError detachFilter(VideoFilterType videoFilterType) override;
  virtual VRError detachAllFilter() override;

  // input Frame
  VRError onInputFrame(VideoFrameMetaData *redRenderBuffer) override;
  // render
  VRError onRender() override;

  VRError onRenderCachedFrame() override;

  static VideoRendererInfo defaultVideoRendererInfo;
  static VideoFilterInfo defaultVideoFilterInfo;

  VRError setGravity(RedRender::GravityResizeAspectRatio rendererGravity);

protected:
  RedRenderMetalView *mMetalView{nullptr};

protected:
  std::map<int /* priority */, MetalFilterBase *>
      _metalFilterChain; /* Store post-processing filters */
  MetalFilterBase *_metalFilterDevice = nullptr; /* Store on-screen filter */
  std::queue<MetalVideoTexture *>
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
  uint64_t _getClock();
  int g_is_mach_base_info_inited;
  kern_return_t g_mach_base_info_ret;
  mach_timebase_info_data_t g_mach_base_info;
};

NS_REDRENDER_END

#endif
#endif /* METAL_VIDEO_RENDERER_H */
