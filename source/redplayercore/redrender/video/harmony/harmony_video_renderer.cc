/*
 * harmony_video_renderer.cpp
 * REDRENDER
 *
 * Copyright (c) 2024 xiaohongshu. All rights reserved.
 * Created by zhangqingyu.
 *
 * This file is part of REDRENDER.
 */
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
#include "./harmony_video_renderer.h"

NS_REDRENDER_BEGIN

HarmonyVideoRenderer::HarmonyVideoRenderer(const int &sessionID)
    : VideoRenderer(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

HarmonyVideoRenderer::~HarmonyVideoRenderer() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

VRError
HarmonyVideoRenderer::onRender(VideoHarmonyDecoderBufferContext *odBufferCtx,
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