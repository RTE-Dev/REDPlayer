/*
 * egl_context.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef EGL_CONTEXT_H
#define EGL_CONTEXT_H

#include "../../video_inc_internal.h"

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>

#include <mutex>
#endif

NS_REDRENDER_BEGIN

// opengl android context class
class EglContext {
public:
  EglContext(const int &sessionID = 0);
  ~EglContext();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
  EGLBoolean init(EGLContext sharedContext = nullptr,
                  EGLNativeWindowType window = nullptr);
  void EGLTerminate();
  EGLBoolean createEGLSurfaceWithWindow(EGLNativeWindowType surface);
  EGLBoolean makeCurrent();
  EGLBoolean makeCurrent(EGLSurface drawSurface, EGLSurface readSurface);
  EGLBoolean setSurfaceSizeWithWindow();
  EGLBoolean setSurfaceSize(int width, int height);
  int getSurfaceWidth();
  int getSurfaceHeight();
  EGLBoolean swapBuffers();

private:
  int _querySurface(int what);
  EGLBoolean _EGLIsValid();
  EGLConfig _getEGLConfig(int version);

  EGLNativeWindowType _window = nullptr;

  EGLDisplay _display = EGL_NO_DISPLAY;
  EGLConfig _config = nullptr;
  EGLContext _context = EGL_NO_CONTEXT;
  EGLSurface _surface = EGL_NO_SURFACE;

  EGLint _width;
  EGLint _height;

  std::mutex _eglMutex;
  const int _sessionID{0};
#endif
};

NS_REDRENDER_END

#endif /* EGL_CONTEXT_H */
