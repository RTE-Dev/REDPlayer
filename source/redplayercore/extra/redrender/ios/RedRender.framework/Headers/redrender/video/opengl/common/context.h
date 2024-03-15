/*
 * context.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "../../video_inc_internal.h"
#include "framebuffer_cache.h"
#include "gl_program.h"
#include <mutex>

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include "../android/egl_context.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../ios/eagl_context.h"
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#endif

NS_REDRENDER_BEGIN

// opengl context class
class GLProgram;
class Context {
public:
  Context(const int &sessionID = 0);
  ~Context();

  static std::shared_ptr<Context> getInstance(const int &sessionID = 0);

  std::shared_ptr<FramebufferCache> getFramebufferCache() const;
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  std::shared_ptr<EglContext> getEglContext() const;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  std::shared_ptr<EaglContext> getEaglContext() const;
#endif
  void setActiveShaderProgram(GLProgram *shaderProgram);
  void clear();
  void opengl_print_string(const char *name, GLenum string);

public:
protected:
  const int _sessionID{0};

private:
  std::shared_ptr<FramebufferCache> _framebufferCache;
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  std::shared_ptr<EglContext> _eglContext;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  std::shared_ptr<EaglContext> _eaglContext;
#endif
  GLProgram *_curShaderProgram;
};

NS_REDRENDER_END

#endif /* CONTEXT_H */
