/*
 * mediacodec_video_renderer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef MEDIACODEC_VIDEO_RENDERER_H
#define MEDIACODEC_VIDEO_RENDERER_H

#include <memory>

#include "../video_filter.h"
#include "../video_filter_info.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

NS_REDRENDER_BEGIN

class MediaCodecVideoRenderer : public VideoRenderer {
public:
  MediaCodecVideoRenderer(
      const VideoRendererInfo &videoRendererInfo = defaultVideoRendererInfo,
      const VideoFilterInfo &videoFilterInfo = defaultVideoFilterInfo,
      const int &sessionID = 0);
  virtual ~MediaCodecVideoRenderer() override;

  virtual VRError
  setBufferProxy(VideoMediaCodecBufferContext *odBufferCtx) override;
  VRError releaseBufferProxy(bool render) override;
  // render
  VRError onRender() override;

  static VideoRendererInfo defaultVideoRendererInfo;
  static VideoFilterInfo defaultVideoFilterInfo;

protected:
  VRError _mediaCodecBufferProxyReset(
      const std::shared_ptr<VideoMediaCodecBufferProxy> &proxy);

protected:
  std::shared_ptr<VideoMediaCodecBufferProxy> _proxy = nullptr;
};

NS_REDRENDER_END

#endif /* MEDIACODEC_VIDEO_RENDERER_H */
