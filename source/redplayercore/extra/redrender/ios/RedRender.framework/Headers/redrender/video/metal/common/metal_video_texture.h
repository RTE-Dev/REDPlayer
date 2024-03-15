/*
 * metal_video_texture.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../video_inc_internal.h"
#import <Metal/Metal.h>

@interface MetalVideoTexture : NSObject

@property(readwrite, nonatomic) id<MTLTexture> texture;
@property(readwrite) uint32_t width;
@property(readwrite) uint32_t height;
@property(readwrite) MTLTextureType textureType;
@property(readwrite) MTLPixelFormat pixelFormat;
@property(readwrite) uint32_t inputTextureIndex;
@property(readwrite) RedRender::RotationMode rotationMode;
@property(readwrite) uint32_t sessionID;

- (id)init:(id<MTLDevice>)device
      sessionID:(uint32_t)sessionID
          width:(uint32_t)imageWidth
         height:(uint32_t)imageHeight
    pixelFormat:(MTLPixelFormat)pixelFormat;
- (id)initWithPixelBuffer:(CVMetalTextureCacheRef)textureCache
              pixelBuffer:(CVPixelBufferRef)pixelBuffer
               planeIndex:(int)planeIndex
                sessionID:(uint32_t)sessionID
              pixelFormat:(MTLPixelFormat)pixelFormat;
- (id)initWithPixelBufferData:(id<MTLDevice>)device
                 textureCache:(CVMetalTextureCacheRef)textureCache
                  pixelBuffer:(CVPixelBufferRef)pixelBuffer
                   planeIndex:(int)planeIndex
                    sessionID:(uint32_t)sessionID
                  pixelFormat:(MTLPixelFormat)pixelFormat;

@end
#endif
