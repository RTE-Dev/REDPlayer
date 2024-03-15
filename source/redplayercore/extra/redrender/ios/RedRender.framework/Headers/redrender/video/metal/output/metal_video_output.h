/*
 * metal_video_output.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_video_cmd_queue.h"
#import "metal_video_input.h"
#import "metal_video_texture.h"
#include "video_inc_internal.h"

@interface MetalVideoOutput : NSObject {
  MetalVideoTexture *outputTexture;
  float outputTextureScale;
  NSMutableArray *targets;
  NSMutableArray *targetTextureIndexs;
  NSMutableArray *outputRotations;
  id<MTLCommandBuffer> sharedcmdBuffer;
}

- (BOOL)newSharedcmdBuffer:(MetalVideoCmdQueue *)metalVideoCmdQueue;
- (void)releaseSharedcmdBuffer;
- (id<MTLCommandBuffer>)getCommandBuffer;
- (void)setInputCommandBuffer:(id<MTLCommandBuffer>)cmdBuffer
                      atIndex:(NSInteger)index;
;
- (void)addTarget:(id<MetalVideoInput>)target;
- (void)addTarget:(id<MetalVideoInput>)target
     rotationMode:(RedRender::RotationMode)rotationMode
     textureIndex:(NSInteger)textureIndex;
- (void)removeTarget:(id<MetalVideoInput>)target;
- (void)removeAllTargets;
- (void)updateTargets:(id<MTLCommandBuffer>)cmdBuffer
            frameTime:(float)frameTime;
- (MetalVideoTexture *)getMetalVideoTexture;
- (void)setOutputRotation:(RedRender::RotationMode)outputRotation;

@end

#endif
