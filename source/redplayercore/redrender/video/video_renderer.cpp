/*
 * video_renderer.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./video_renderer.h"

NS_REDRENDER_BEGIN

VideoRenderer::VideoRenderer(const int &sessionID) : _sessionID(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

VideoRenderer::~VideoRenderer() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

// OpenGL
VRError VideoRenderer::releaseContext() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
VRError VideoRenderer::init(ANativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::setSurface(ANativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
VRError VideoRenderer::init(OHNativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError
VideoRenderer::setSurface(OHNativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
VRError VideoRenderer::init() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::initWithFrame(CGRect cgrect /*= {{0,0}, {0,0}} */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

UIView *VideoRenderer::getRedRenderView() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return nullptr;
}
#endif

VRError VideoRenderer::attachFilter(
    VideoFilterType videoFilterType,
    VideoFrameMetaData *inputFrameMetaData /* = nullptr */,
    int priority /* = -1 */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::detachFilter(VideoFilterType videoFilterType) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::detachAllFilter() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::onUpdateParam(VideoFilterType videoFilterType) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::onInputFrame(VideoFrameMetaData *redRenderBuffer) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
// MediaCodec interface
VRError VideoRenderer::onRender(VideoMediaCodecBufferContext *odBufferCtx,
                                bool render) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
// HarmonyDecoder interface
VRError VideoRenderer::onRender(VideoHarmonyDecoderBufferContext *odBufferCtx,
                                bool render) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}
#endif

// common interface
VRError VideoRenderer::onRender() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

VRError VideoRenderer::onRenderCachedFrame() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

NS_REDRENDER_END
