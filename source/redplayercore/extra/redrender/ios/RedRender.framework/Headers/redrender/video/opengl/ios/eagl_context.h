/*
 * eagl_context.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef EAGL_CONTEXT_H
#define EAGL_CONTEXT_H

#include "../../video_inc_internal.h"
#include <functional>

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "redrender_gl_texture.h"
#import "redrender_gl_view.h"
#import <OpenGLES/EAGL.h>
#import <UIKit/UIKit.h>
#define GL_CONTEXT_QUEUE "RedRender.OpenGLESContextQueue"

#endif

NS_REDRENDER_BEGIN

// opengl ios context class
class EaglContext {
public:
  EaglContext(const int &sessionID = 0);
  ~EaglContext();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  bool init(CGRect cgrect, void *renderer);
  void useAsCurrent();
  void useAsPrev();
  void presentBufferForDisplay();
  void activeFramebuffer();
  void inActiveFramebuffer();
  int getFrameWidth();
  int getFrameHeight();
  RedRenderGLTexture *
  getGLTextureWithPixelBuffer(CVOpenGLESTextureCacheRef textureCache,
                              CVPixelBufferRef pixelBuffer, int planeIndex,
                              uint32_t sessionID, GLenum target,
                              GLenum pixelFormat, GLenum type);
  VRAVColorSpace getColorSpace(CVPixelBufferRef pixelBuffer);
  RedRenderGLView *getRedRenderGLView();
  bool isDisplay();

private:
  //    EAGLContext* _context;
  RedRenderGLView *_redRenderGLView{nullptr};
  const int _sessionID{0};
#endif
};

NS_REDRENDER_END

#endif /* EAGL_CONTEXT_H */
