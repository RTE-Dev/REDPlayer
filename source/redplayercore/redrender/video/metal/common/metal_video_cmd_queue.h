#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../video_inc_internal.h"
#import <Metal/Metal.h>

@interface MetalVideoCmdQueue : NSObject

@property(readonly, nonatomic) id<MTLDevice> device;
@property(readonly, nonatomic) id<MTLCommandQueue> commandQueue;
@property(readonly, nonatomic) dispatch_semaphore_t cmdBufferProcessSemaphore;
@property(readonly, nonatomic) CVMetalTextureCacheRef textureCache;
@property(readwrite) uint32_t sessionID;

+ (id)getCmdQueueInstance;
- (id<MTLCommandBuffer>)newCmdBuffer;

@end
#endif
