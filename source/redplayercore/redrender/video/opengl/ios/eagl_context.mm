/*
 * eagl_context.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#include "eagl_context.h"
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "redrender_gl_view.h"
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>
#endif

NS_REDRENDER_BEGIN

EaglContext::EaglContext(const int &sessionID) : _sessionID(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

EaglContext::~EaglContext() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _redRenderGLView = nil;
}

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
bool EaglContext::init(CGRect cgrect, void *renderer) {
  _redRenderGLView = [[RedRenderGLView alloc] initWithFrame:cgrect
                                                  sessionID:_sessionID
                                                   renderer:renderer];
  return true;
}

void EaglContext::useAsCurrent() { [_redRenderGLView useAsCurrent]; }

void EaglContext::useAsPrev() { [_redRenderGLView useAsPrev]; }

void EaglContext::presentBufferForDisplay() {
  [_redRenderGLView presentBufferForDisplay];
}

void EaglContext::activeFramebuffer() { [_redRenderGLView activeFramebuffer]; }

void EaglContext::inActiveFramebuffer() {
  [_redRenderGLView inActiveFramebuffer];
}

int EaglContext::getFrameWidth() { return [_redRenderGLView getFrameWidth]; }

int EaglContext::getFrameHeight() { return [_redRenderGLView getFrameHeigh]; }

RedRenderGLTexture *EaglContext::getGLTextureWithPixelBuffer(
    CVOpenGLESTextureCacheRef textureCache, CVPixelBufferRef pixelBuffer,
    int planeIndex, uint32_t sessionID, GLenum target, GLenum pixelFormat,
    GLenum type) {
  RedRenderGLTexture *ptr =
      [[RedRenderGLTexture alloc] initWithPixelBuffer:textureCache
                                          pixelBuffer:pixelBuffer
                                           planeIndex:planeIndex
                                            sessionID:sessionID
                                               target:target
                                          pixelFormat:pixelFormat
                                                 type:type];
  if (nullptr == ptr) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] error .\n", __FUNCTION__,
               __LINE__);
    return nullptr;
  }
  return ptr;
}

VRAVColorSpace EaglContext::getColorSpace(CVPixelBufferRef pixelBuffer) {
  if (_redRenderGLView) {
    return [_redRenderGLView getColorSpace:pixelBuffer];
  }
  return VR_AVCOL_SPC_NB;
}

RedRenderGLView *EaglContext::getRedRenderGLView() { return _redRenderGLView; }

bool EaglContext::isDisplay() {
  if (_redRenderGLView) {
    return [_redRenderGLView isDisplay];
  }
  return NO;
}

#endif

NS_REDRENDER_END
