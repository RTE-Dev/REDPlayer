/*
 * avsbdl_video_renderer.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "avsbdl_video_renderer.h"
#include "RedBase.h"
NS_REDRENDER_BEGIN

AVSBDLVideoRenderer::AVSBDLVideoRenderer(const int &sessionID)
    : VideoRenderer(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

AVSBDLVideoRenderer::~AVSBDLVideoRenderer() {
  mAVSBDLView = nil;
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

VRError AVSBDLVideoRenderer::initWithFrame(CGRect cgrect) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  mAVSBDLView = [[RedRenderAVSBDLView alloc] initWithFrame:cgrect
                                                 sessionID:_sessionID];
  [mAVSBDLView setLayerSessionID:_sessionID];
  return VRError::VRErrorNone;
}

UIView *AVSBDLVideoRenderer::getRedRenderView() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return mAVSBDLView;
}

// input frames for post-processing, and the result is stored in the
// queue:_textureBufferQueue
VRError AVSBDLVideoRenderer::onInputFrame(VideoFrameMetaData *redRenderBuffer) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
  VRError ret = VRError::VRErrorNone;
  if (!redRenderBuffer) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] redRenderBuffer is null.\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInputFrame;
  }

  if (redRenderBuffer->pixel_format != RedRender::VRPixelFormatYUV420sp_vtb) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] pixel_format is %d, not support!\n", __FUNCTION__,
               __LINE__, redRenderBuffer->pixel_format);
    return VRError::VRErrorInputFrame;
  }

  ret = _setInputFrame(redRenderBuffer);
  if (ret != VRError::VRErrorNone) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _setInputImage error.\n",
               __FUNCTION__, __LINE__);
    return ret;
  }
  return VRError::VRErrorNone;
}

// take frames from the queue:_textureBufferQueue and render them on the screen
VRError AVSBDLVideoRenderer::onRender() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
  @autoreleasepool {
    if (mInputVideoFrameMetaData->pixel_format !=
        RedRender::VRPixelFormatYUV420sp_vtb) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] pixel_format is %d, not support!\n", __FUNCTION__,
                 __LINE__, mInputVideoFrameMetaData->pixel_format);
      return VRError::VRErrorOnRender;
    }
    _on_screen_render();
  }
  return VRError::VRErrorNone;
}

VRError
AVSBDLVideoRenderer::_setInputFrame(VideoFrameMetaData *inputFrameMetaData) {
  mInputVideoFrameMetaData = inputFrameMetaData;
  return VRError::VRErrorNone;
}

VRError AVSBDLVideoRenderer::_on_screen_render() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);

  [mAVSBDLView onRender:mInputVideoFrameMetaData->pixel_buffer];
  // fps
  uint64_t current = CurrentTimeMs();
  uint64_t delta = (current > _lastFrameTime) ? current - _lastFrameTime : 0;
  if (delta <= 0) {
    _lastFrameTime = current;
  } else if (delta >= 1000) {
    _fps = ((double)_frameCount) * 1000 / delta;
    _lastFrameTime = current;
    _frameCount = 0;
  } else {
    _frameCount++;
  }
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] render fps : %lf .\n", __FUNCTION__,
             __LINE__, _fps);
  return VRError::VRErrorNone;
}

NS_REDRENDER_END
#endif
