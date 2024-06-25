/*
 * video_renderer_factory.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu
 *
 * This file is part of RedRender.
 */
#include "./video_renderer_factory.h"

#include "avsbdl/avsbdl_video_renderer.h"
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include "mediacodec/mediacodec_video_renderer.h"
#endif
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
#include "harmony/harmony_video_renderer.h"
#endif
#include "metal/metal_video_renderer.h"
#include "opengl/opengl_video_renderer.h"

NS_REDRENDER_BEGIN

VideoRendererFactory::VideoRendererFactory() {
  AV_LOGV(LOG_TAG, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _supportedRenderer = {
      VideoRendererInfo(VRClusterType::VRClusterTypeOpenGL),
      VideoRendererInfo(VRClusterType::VRClusterTypeMetal),
      VideoRendererInfo(VRClusterType::VRClusterTypeMediaCodec),
      VideoRendererInfo(VRClusterType::VRClusterTypeAVSBDL),
      VideoRendererInfo(VRClusterType::VRClusterTypeHarmonyVideoDecoder),
  };
}

VideoRendererFactory::~VideoRendererFactory() {
  AV_LOGV(LOG_TAG, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

std::unique_ptr<VideoRenderer> VideoRendererFactory::createVideoRenderer(
    const VideoRendererInfo &videoRendererInfo, const int &sessionID) {
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_ptr<VideoRenderer> renderer = nullptr;
  if (!isSupportedRender(videoRendererInfo)) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] not support!\n", __FUNCTION__,
               __LINE__);
    return renderer;
  }

  switch (videoRendererInfo._videoRendererClusterType) {
  case VRClusterTypeOpenGL:
    renderer = std::make_unique<OpenGLVideoRenderer>(sessionID);
    break;
  case VRClusterTypeMetal:
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
    renderer = std::make_unique<MetalVideoRenderer>(sessionID);
#endif
    break;
  case VRClusterTypeMediaCodec:
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
    renderer = std::make_unique<MediaCodecVideoRenderer>(sessionID);
#endif
    break;
  case VRClusterTypeHarmonyVideoDecoder:
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
    renderer = std::make_unique<HarmonyVideoRenderer>(sessionID);
#endif
    break;
  case VRClusterTypeAVSBDL:
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
    renderer = std::make_unique<AVSBDLVideoRenderer>(sessionID);
#endif
    break;
  case VRClusterTypeUnknown:
  default:
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] VideoRendererTypeUnknown.\n",
               __FUNCTION__, __LINE__);
    break;
  }
  if (renderer != nullptr) {
    AV_LOGI_ID(LOG_TAG, sessionID,
               "[%s:%d] renderer create success, type:%d .\n", __FUNCTION__,
               __LINE__, videoRendererInfo._videoRendererClusterType);
  }

  return renderer;
}

bool VideoRendererFactory::isSupportedRender(
    const VideoRendererInfo &videoRendererInfo) {
  bool ret = std::any_of(_supportedRenderer.begin(), _supportedRenderer.end(),
                         [&](const VideoRendererInfo &renderer) {
                           return renderer._videoRendererClusterType ==
                                  videoRendererInfo._videoRendererClusterType;
                         });
  return ret;
}

NS_REDRENDER_END
