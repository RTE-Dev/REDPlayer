/*
 * video_renderer_info.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./video_renderer_info.h"

NS_REDRENDER_BEGIN

VideoRendererInfo::VideoRendererInfo(VRClusterType rendererType)
    : _videoRendererClusterType(rendererType) {
  AV_LOGV(LOG_TAG, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

NS_REDRENDER_END
