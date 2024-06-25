#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include <mach/mach_time.h>
#include <mutex>
#include <sys/time.h>
#include <time.h>

#include "../video_filter.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"
#include "../video_renderer_info.h"

#include "../video_renderer_factory.h"
#import "redrender_avsbdl_view.h"

NS_REDRENDER_BEGIN

class AVSBDLVideoRenderer : public VideoRenderer {
public:
  AVSBDLVideoRenderer(const int &sessionID);
  ~AVSBDLVideoRenderer() override;
  VRError initWithFrame(CGRect cgrect = {{0, 0}, {0, 0}}) override;
  UIView *getRedRenderView() override;

  // input Frame
  VRError onInputFrame(VideoFrameMetaData *redRenderBuffer) override;
  // render
  VRError onRender() override;

protected:
  RedRenderAVSBDLView *mAVSBDLView{nullptr};
  VideoFrameMetaData *mInputVideoFrameMetaData{nullptr};

private:
  VRError _setInputFrame(VideoFrameMetaData *inputFrameMetaData);
  VRError _post_processing();
  VRError _on_screen_render();

  std::mutex _rendererMutex;

  // fps
  double _fps;
  uint64_t _lastFrameTime = 0;
  int _frameCount = 0;
};

NS_REDRENDER_END

#endif
