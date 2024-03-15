/*
 * avsbdl_video_renderer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef AVSBDL_VIDEO_RENDERER_H
#define AVSBDL_VIDEO_RENDERER_H

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include <mach/mach_time.h>
#include <mutex>
#include <sys/time.h>
#include <time.h>

#include "../video_filter.h"
#include "../video_filter_info.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

#import "redrender_avsbdl_view.h"
#include "video_renderer_factory.h"

NS_REDRENDER_BEGIN

class AVSBDLVideoRenderer : public VideoRenderer {
public:
  AVSBDLVideoRenderer(
      const VideoRendererInfo &videoRendererInfo = defaultVideoRendererInfo,
      const VideoFilterInfo &videoFilterInfo = defaultVideoFilterInfo,
      const int &sessionID = 0);
  virtual ~AVSBDLVideoRenderer() override;
  virtual VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
  virtual UIView *getRedRenderView() override;

  // input Frame
  VRError onInputFrame(VideoFrameMetaData *redRenderBuffer) override;
  // render
  VRError onRender() override;

  static VideoRendererInfo defaultVideoRendererInfo;
  static VideoFilterInfo defaultVideoFilterInfo;

protected:
  RedRenderAVSBDLView *mAVSBDLView{nullptr};
  VideoFrameMetaData *mInputVideoFrameMetaData{nullptr};
  std::shared_ptr<RedRender::VideoRenderer> mMetalVideoRender{nullptr};

private:
  VRError _setInputFrame(VideoFrameMetaData *inputFrameMetaData);
  VRError _post_processing();
  VRError _on_screen_render();

  std::mutex _rendererMutex;

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
#endif /* AVSBDL_VIDEO_RENDERER_H */
