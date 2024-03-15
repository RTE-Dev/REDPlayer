/*
 * yuv2rgb_filter.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_filter_base.h"
#include "video_inc_internal.h"

@interface MetalYUV2RGBFilter : MetalFilterBase

- (id)init:(MetalVideoCmdQueue *)metalVideoCmdQueue
    inputFrameMetaData:(RedRender::VideoFrameMetaData *)inputFrameMetaData
             sessionID:(int)sessionID;

@end
#endif
