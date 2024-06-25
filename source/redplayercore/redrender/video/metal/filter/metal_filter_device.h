#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../video_inc_internal.h"
#import "metal_filter_base.h"

@interface MetalFilterDevice : MetalFilterBase

- (id)init:(MetalVideoCmdQueue *)metalVideoCmdQueue
    inputFrameMetaData:(RedRender::VideoFrameMetaData *)inputFrameMetaData
             sessionID:(int)sessionID;
- (void)onRenderDrawInMTKView:(RedRenderMetalView *)view;

@end
#endif
