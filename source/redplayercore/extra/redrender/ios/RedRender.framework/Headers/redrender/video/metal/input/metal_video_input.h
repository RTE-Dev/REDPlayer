/*
 * metal_video_input.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_video_texture.h"
#include "video_inc_internal.h"
#import <Metal/Metal.h>

@protocol MetalVideoInput <NSObject>

- (void)setInputCmdBuffer:(id<MTLCommandBuffer>)cmdBuffer
                  atIndex:(NSInteger)index;
- (void)setInputTexture:(MetalVideoTexture *)inputTexture
           rotationMode:(RedRender::RotationMode)rotationMode
      inputTextureIndex:(int)inputTextureIndex;
- (void)clearInputTexture;
- (NSInteger)getNextAvailableTextureIndex;
- (bool)isPrepared;

@end
#endif
