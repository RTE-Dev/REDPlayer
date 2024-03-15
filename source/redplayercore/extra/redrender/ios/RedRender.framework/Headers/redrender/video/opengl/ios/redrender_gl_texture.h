/*
 * redrender_gl_texture.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef REDRENDER_GL_TEXTURE_H
#define REDRENDER_GL_TEXTURE_H

#include "../../video_inc_internal.h"
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import <UIKit/UIKit.h>

@interface RedRenderGLTexture : NSObject

@property(readwrite, nonatomic) GLuint texture;
@property(readwrite) GLsizei width;
@property(readwrite) GLsizei height;
@property(readwrite) GLenum textureTarget;
@property(readwrite) GLenum pixelFormat;
@property(readwrite) GLenum textureType;
@property(readwrite) uint32_t inputTextureIndex;
@property(readwrite) RedRender::RotationMode rotationMode;
@property(readwrite) uint32_t sessionID;

- (id)initWithPixelBuffer:(CVOpenGLESTextureCacheRef)textureCache
              pixelBuffer:(CVPixelBufferRef)pixelBuffer
               planeIndex:(int)planeIndex
                sessionID:(uint32_t)sessionID
                   target:(GLenum)target
              pixelFormat:(GLenum)pixelFormat
                     type:(GLenum)type;

@end

#endif
#endif /* REDRENDER_GL_TEXTURE_H */
