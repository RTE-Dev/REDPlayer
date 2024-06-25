#pragma once

#include "./video_inc_internal.h"

NS_REDRENDER_BEGIN

typedef enum VRClusterType {
  VRClusterTypeUnknown = 0,
  VRClusterTypeOpenGL = 1,
  VRClusterTypeMetal = 2,
  VRClusterTypeMediaCodec = 3,
  VRClusterTypeAVSBDL = 5, // AVSampleBufferDisplayLayer
  VRClusterTypeHarmonyVideoDecoder = 6,
} VRClusterType;

struct VideoRendererInfo {
  VideoRendererInfo(VRClusterType rendererType = VRClusterTypeUnknown);
  ~VideoRendererInfo() = default;
  VRClusterType _videoRendererClusterType;
};

NS_REDRENDER_END
