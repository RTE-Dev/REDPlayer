/*
 * video_renderer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "../common/redrender_buffer.h"
#include "opengl/filter/opengl_filter_base.h"
#include "video_filter.h"
#include "video_filter_info.h"
#include "video_inc_internal.h"
#include "video_renderer_info.h"
#include <memory>

NS_REDRENDER_BEGIN

class VideoRendererCallback {
public:
  virtual ~VideoRendererCallback() = default;
  //    virtual VideoRenderError
  //    VideoRenderFrame(std::unique_ptr<RedRenderBuffer> frame);
  virtual void VideoRenderError(VRError error);
};

// renderer base class
class VideoRenderer {
public:
  VideoRenderer(const std::string rendererName,
                const VideoRendererInfo &videoRendererInfo,
                const VideoFilterInfo &videoFilterInfo,
                const int &sessionID = 0);
  virtual ~VideoRenderer();
  virtual VRError
  registerVideoRendererCallback(VideoRendererCallback *callback = nullptr,
                                bool freeCallbackWhenRelease = false);
  virtual VideoRendererCallback *getVideoRendererCallback();

  // OpenGL interface
  virtual VRError releaseContext();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  virtual VRError init(ANativeWindow *nativeWindow = nullptr);
  virtual VRError setSurface(ANativeWindow *nativeWindow = nullptr);
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

  // MediaCodec interface
  virtual VRError setBufferProxy(VideoMediaCodecBufferContext *odBufferCtx);
  // HisiMedia interface
  virtual VRError setBufferProxy(VideoHisiMediaBufferContext *odBufferCtx);
  virtual VRError releaseBufferProxy(bool render);
  // Common interface
  virtual VRError onRender();
  virtual VRError onRenderCachedFrame();
  std::string getRendererName() { return mRendererName; };

protected:
  VideoRendererCallback *_videoRendererCB{nullptr};
  bool _freeCallbackWhenRelease{false};
  std::string mRendererName = "";
  VideoRendererInfo _videoRendererInfo;
  VideoFilterInfo _videoFilterInfo;
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* VIDEO_RENDERER_H */
