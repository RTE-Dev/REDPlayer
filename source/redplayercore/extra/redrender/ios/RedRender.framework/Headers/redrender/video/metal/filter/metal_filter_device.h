/*
 * metal_filter_device.h
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

@interface MetalFilterDevice : MetalFilterBase

- (id)init:(MetalVideoCmdQueue *)metalVideoCmdQueue
    inputFrameMetaData:(RedRender::VideoFrameMetaData *)inputFrameMetaData
             sessionID:(int)sessionID;
- (void)onRenderDrawInMTKView:(RedRenderMetalView *)view;

@end
#endif
