/*
 * redrender_gl_texture.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "redrender_gl_texture.h"

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>

@interface RedRenderGLTexture () {
}
@end

@implementation RedRenderGLTexture

- (id)initWithPixelBuffer:(CVOpenGLESTextureCacheRef)textureCache
              pixelBuffer:(CVPixelBufferRef)pixelBuffer
               planeIndex:(int)planeIndex
                sessionID:(uint32_t)sessionID
                   target:(GLenum)target
              pixelFormat:(GLenum)pixelFormat
                     type:(GLenum)type {
  if (!(self = [super init]) || pixelBuffer == nil) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] RedRenderGLTexture init error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }

  _sessionID = sessionID;
  _textureTarget = target;
  _pixelFormat = pixelFormat;
  _textureType = type;
  _inputTextureIndex = 0;
  _rotationMode = RedRender::NoRotation;
  _texture = 0;

  _width = (GLsizei)CVPixelBufferGetWidthOfPlane(pixelBuffer, planeIndex);
  _height = (GLsizei)CVPixelBufferGetHeightOfPlane(pixelBuffer, planeIndex);
  CVOpenGLESTextureRef texture = nil;
  CVReturn status = CVOpenGLESTextureCacheCreateTextureFromImage(
      kCFAllocatorDefault, textureCache, pixelBuffer, NULL, target, pixelFormat,
      _width, _height, pixelFormat, type, planeIndex, &texture);
  if (status == kCVReturnSuccess) {
    _texture = CVOpenGLESTextureGetName(texture);
    CFRelease(texture);
    texture = nil;
    return self;
  } else {
    AV_LOGE_ID(LOG_TAG, sessionID,
               "[%s:%d] RedRenderGLTexture init error:%d .\n", __FUNCTION__,
               __LINE__, (int32_t)status);
    return nil;
  }

  return self;
}

- (void)dealloc {
  if (_texture > 0) {
    _texture = 0;
  }
}

@end

#endif
