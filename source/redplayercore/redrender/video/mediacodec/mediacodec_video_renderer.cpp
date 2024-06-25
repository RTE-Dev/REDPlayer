/*
 * mediacodec_video_renderer.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include "./mediacodec_video_renderer.h"
NS_REDRENDER_BEGIN

MediaCodecVideoRenderer::MediaCodecVideoRenderer(const int &sessionID)
    : VideoRenderer(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

MediaCodecVideoRenderer::~MediaCodecVideoRenderer() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

VRError
MediaCodecVideoRenderer::onRender(VideoMediaCodecBufferContext *odBufferCtx,
                                  bool render) {
  if (nullptr == odBufferCtx) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] odBufferCtx is null.\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInit;
  }
  if (nullptr == odBufferCtx->release_output_buffer) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] odBufferCtx.release_output_buffer is null.\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInit;
  }
  odBufferCtx->release_output_buffer(odBufferCtx, render);
  return VRError::VRErrorNone;
}

NS_REDRENDER_END
#endif