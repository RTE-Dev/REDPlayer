/*
 * video_renderer_info.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef VIDEO_RENDERER_INFO_H
#define VIDEO_RENDERER_INFO_H

#include "video_inc_internal.h"

NS_REDRENDER_BEGIN

typedef enum VRClusterType {
  VRClusterTypeUnknown = 0,
  VRClusterTypeOpenGL = 1,
  VRClusterTypeMetal = 2,
  VRClusterTypeMediaCodec = 3,
  VRClusterTypeAVSBDL = 5, // AVSampleBufferDisplayLayer
                           // todo : add Vulkan, DX11
} VRClusterType;

class VideoRendererInfo {
public:
  VideoRendererInfo(VRClusterType rendererType = VRClusterTypeUnknown);
  ~VideoRendererInfo() = default;

  VRClusterType _videoRendererClusterType;
};

NS_REDRENDER_END

#endif /* VIDEO_RENDERER_INFO_H */
