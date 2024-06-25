/*
 * egl_context.cpp
 * REDRENDER
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of REDRENDER.
 */
#include "./egl_context.h"
#include <native_window/external_window.h>

NS_REDRENDER_BEGIN

EglContext::EglContext(const int &sessionID) : _sessionID(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

EglContext::~EglContext() {
  EGLTerminate();
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
EGLBoolean EglContext::init(EGLContext sharedContext /* = nullptr */,
                            OHNativeWindow *window /* = nullptr */) {
  std::unique_lock<std::mutex> lck(_eglMutex);
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (_EGLIsValid()) {
    return EGL_TRUE;
  }

  EGLTerminate();
  if (nullptr == sharedContext) {
    sharedContext = EGL_NO_CONTEXT;
  }

  _display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (_display == EGL_NO_DISPLAY) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] [EGL] eglGetDisplay failed .\n",
               __FUNCTION__, __LINE__);
    return EGL_FALSE;
  }

  EGLint major, minor;
  if (!eglInitialize(_display, &major, &minor)) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] [EGL] eglInitialize failed .\n",
               __FUNCTION__, __LINE__);
    return EGL_FALSE;
  }
  AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] [EGL] eglInitialize %d.%d .\n",
             __FUNCTION__, __LINE__, (int)major, (int)minor);
  int version = 3;
  EGLConfig config = _getEGLConfig(version);
  if (nullptr != config) {
    EGLint context3Attribs[] = {EGL_CONTEXT_CLIENT_VERSION, version, EGL_NONE};
    _context =
        eglCreateContext(_display, config, sharedContext, context3Attribs);
    if (_context == EGL_NO_CONTEXT) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] [EGL] eglCreateContext version 3: error=0x%04X .\n",
                 __FUNCTION__, __LINE__, eglGetError());
      version = 2;
      config = _getEGLConfig(version);
      if (nullptr != config) {
        EGLint context2Attribs[] = {EGL_CONTEXT_CLIENT_VERSION, version,
                                    EGL_NONE};
        _context =
            eglCreateContext(_display, config, sharedContext, context2Attribs);
        if (_context == EGL_NO_CONTEXT) {
          AV_LOGE_ID(
              LOG_TAG, _sessionID,
              "[%s:%d] [EGL] eglCreateContext version 2: error=0x%04X .\n",
              __FUNCTION__, __LINE__, eglGetError());
          eglTerminate(_display);
          return EGL_FALSE;
        }
      }
    }
    _config = config;
  }
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] versione=%d, _display:%p, _surface:%p, _context:%p, "
             "_window:%p.\n",
             __FUNCTION__, __LINE__, version, _display, _surface, _context,
             _window);

  if (window) {
    EGLint surfaceAttribs[] = {EGL_NONE};
    _surface = eglCreateWindowSurface(
        _display, config, (EGLNativeWindowType)window, surfaceAttribs);
    if (EGL_NO_SURFACE == _surface) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] [EGL] eglCreateWindowSurface failed error=0x%04X .\n",
                 __FUNCTION__, __LINE__, eglGetError());
      eglTerminate(_display);
      return EGL_FALSE;
    }
    _window = window;

    EGLBoolean res = setSurfaceSizeWithWindow();
    if (!res) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] setSurfaceSizeWithWindow failed .\n", __FUNCTION__,
                 __LINE__);
      return EGL_FALSE;
    }

    res = eglMakeCurrent(_display, _surface, _surface, _context);
    if (!res) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                 __FUNCTION__, __LINE__, res, eglGetError());
      if (_surface != EGL_NO_SURFACE) {
        res = eglDestroySurface(_display, _surface);
        if (!res) {
          AV_LOGE_ID(
              LOG_TAG, _sessionID,
              "[%s:%d] eglDestroySurface failed res=%d , error=0x%04X .\n",
              __FUNCTION__, __LINE__, res, eglGetError());
        }
      }

      if (_context != EGL_NO_CONTEXT) {
        res = eglDestroyContext(_display, _context);
        if (!res) {
          AV_LOGE_ID(
              LOG_TAG, _sessionID,
              "[%s:%d] eglDestroyContext failed res=%d , error=0x%04X .\n",
              __FUNCTION__, __LINE__, res, eglGetError());
        }
      }
      eglTerminate(_display);
      return EGL_FALSE;
    }
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
               __FUNCTION__, __LINE__, _display, _surface, _context, _window);
  }
  return EGL_TRUE;
}

void EglContext::EGLTerminate() {
  if (!_EGLIsValid()) {
    return;
  }

  if (_display) {
    glFinish();
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
               __FUNCTION__, __LINE__, _display, _surface, _context, _window);

    EGLContext currentContext = eglGetCurrentContext();
    EGLSurface currentDrawSurface = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface currentReadSurface = eglGetCurrentSurface(EGL_READ);
    EGLDisplay currentDisplay = eglGetCurrentDisplay();
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] curDisplay:%p, curDrawSurface:%p, curReadSurface:%p, "
               "curContext:%p.\n",
               __FUNCTION__, __LINE__, currentDisplay, currentDrawSurface,
               currentReadSurface, currentContext);

    if (currentContext != EGL_NO_CONTEXT &&
        currentDrawSurface != EGL_NO_SURFACE &&
        currentReadSurface != EGL_NO_SURFACE &&
        currentDisplay != EGL_NO_DISPLAY && currentContext == _context &&
        currentDrawSurface == _surface && currentReadSurface == _surface &&
        currentDisplay == _display) {
      AV_LOGI_ID(LOG_TAG, _sessionID,
                 "[%s:%d] the egl environment is consistent .\n", __FUNCTION__,
                 __LINE__);
      EGLBoolean res = eglMakeCurrent(currentDisplay, currentDrawSurface,
                                      currentReadSurface, currentContext);
      if (!res) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] eglMakeCurrent failed .\n",
                   __FUNCTION__, __LINE__);
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                   __FUNCTION__, __LINE__, res, eglGetError());
      }
    } else {
      AV_LOGI_ID(LOG_TAG, _sessionID,
                 "[%s:%d] the egl environment is inconsistent .\n",
                 __FUNCTION__, __LINE__);
      if (currentContext == EGL_NO_CONTEXT ||
          currentDrawSurface == EGL_NO_SURFACE ||
          currentReadSurface == EGL_NO_SURFACE ||
          currentDisplay == EGL_NO_DISPLAY) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglGetCurrentContext failed .\n", __FUNCTION__,
                   __LINE__);
        return;
      }
      EGLBoolean res = eglMakeCurrent(currentDisplay, currentDrawSurface,
                                      currentReadSurface, currentContext);
      if (!res) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] eglMakeCurrent failed .\n",
                   __FUNCTION__, __LINE__);
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                   __FUNCTION__, __LINE__, res, eglGetError());
        return;
      }
    }

    EGLBoolean res = eglMakeCurrent(currentDisplay, EGL_NO_SURFACE,
                                    EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!res) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
                 __FUNCTION__, __LINE__, currentDisplay, currentDrawSurface,
                 currentContext, _window);
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                 __FUNCTION__, __LINE__, res, eglGetError());
    }

    if (currentDrawSurface != EGL_NO_SURFACE) {
      res = eglDestroySurface(currentDisplay, currentDrawSurface);
      if (!res) {
        AV_LOGE_ID(
            LOG_TAG, _sessionID,
            "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
            __FUNCTION__, __LINE__, currentDisplay, currentDrawSurface,
            currentContext, _window);
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                   __FUNCTION__, __LINE__, res, eglGetError());
      }
    }

    if (currentReadSurface != EGL_NO_SURFACE &&
        currentReadSurface != currentDrawSurface) {
      res = eglDestroySurface(currentDisplay, currentReadSurface);
      if (!res) {
        AV_LOGE_ID(
            LOG_TAG, _sessionID,
            "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
            __FUNCTION__, __LINE__, currentDisplay, currentReadSurface,
            currentContext, _window);
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                   __FUNCTION__, __LINE__, res, eglGetError());
      }
    }

    if (currentContext != EGL_NO_CONTEXT) {
      res = eglDestroyContext(currentDisplay, currentContext);
      if (!res) {
        AV_LOGE_ID(
            LOG_TAG, _sessionID,
            "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
            __FUNCTION__, __LINE__, currentDisplay, currentDrawSurface,
            currentContext, _window);
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                   __FUNCTION__, __LINE__, res, eglGetError());
      }
    }

    eglTerminate(currentDisplay);
    eglReleaseThread(); // FIXME: call at thread exit
  }
  _display = EGL_NO_DISPLAY;
  _context = EGL_NO_CONTEXT;
  _surface = EGL_NO_SURFACE;
  if (_window) {
    OH_NativeWindow_NativeObjectUnreference(_window);
    _window = nullptr;
  }
}

EGLBoolean EglContext::createEGLSurfaceWithWindow(OHNativeWindow *window) {
  std::unique_lock<std::mutex> lck(_eglMutex);
  if (_window) {
    if (_window == window) {
      AV_LOGI_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _display:%p, _surface:%p, _context:%p, old _window "
                 "== new window:%p.\n",
                 __FUNCTION__, __LINE__, _display, _surface, _context, window);
      EGLBoolean res = setSurfaceSizeWithWindow();
      if (!res) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] setSurfaceSizeWithWindow failed .\n", __FUNCTION__,
                   __LINE__);
        return EGL_FALSE;
      }

      if (!makeCurrent()) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], makeCurrent error .\n",
                   __FUNCTION__, __LINE__);
        return EGL_FALSE;
      }
      return EGL_TRUE;
    }
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] _display:%p, _surface:%p, _context:%p, old _window:%p "
               "!=  new window:%p.\n",
               __FUNCTION__, __LINE__, _display, _surface, _context, _window,
               window);
    OH_NativeWindow_NativeObjectUnreference(_window);
    _window = nullptr;
    EGLBoolean res = eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                    EGL_NO_CONTEXT);
    if (!res) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                 __FUNCTION__, __LINE__, res, eglGetError());
      return EGL_FALSE;
    }

    if (_surface) {
      res = eglDestroySurface(_display, _surface);
      if (!res) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
                   __FUNCTION__, __LINE__, res, eglGetError());
      }
      _surface = EGL_NO_SURFACE;
    }
  }
  OH_NativeWindow_NativeObjectReference(window);
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] _display:%p, _surface:%p, _context:%p, window:%p.\n",
             __FUNCTION__, __LINE__, _display, _surface, _context, window);
  if (!_surface) {
    EGLint surfaceAttribs[] = {EGL_NONE};
    _surface = eglCreateWindowSurface(
        _display, _config, (EGLNativeWindowType)window, surfaceAttribs);
    if (!_surface) {
      AV_LOGE_ID(
          LOG_TAG, _sessionID,
          "[%s:%d] [EGL] eglCreateWindowSurface failed , error=0x%04X .\n",
          __FUNCTION__, __LINE__, eglGetError());
      return EGL_FALSE;
    }
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] create surface success _display:%p, _surface:%p, "
               "_context:%p, _window:%p.\n",
               __FUNCTION__, __LINE__, _display, _surface, _context, _window);
    _window = window;
  }

  EGLBoolean res = setSurfaceSizeWithWindow();
  if (!res) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] setSurfaceSizeWithWindow failed .\n", __FUNCTION__,
               __LINE__);
    return EGL_FALSE;
  }

  if (!makeCurrent()) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], makeCurrent error .\n",
               __FUNCTION__, __LINE__);
    return EGL_FALSE;
  }

  return EGL_TRUE;
}

EGLBoolean EglContext::makeCurrent() {
  if (EGL_NO_DISPLAY == _display) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] no display .\n", __FUNCTION__,
               __LINE__);
    return EGL_FALSE;
  }

  if (EGL_NO_SURFACE == _surface) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] no surface .\n", __FUNCTION__,
               __LINE__);
    return EGL_FALSE;
  }

  if (EGL_NO_CONTEXT == _context) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] no context .\n", __FUNCTION__,
               __LINE__);
    return EGL_FALSE;
  }

  EGLBoolean res = eglMakeCurrent(_display, _surface, _surface, _context);
  if (!res) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] _display:%p, _surface:%p, _context:%p, _window:%p.\n",
               __FUNCTION__, __LINE__, _display, _surface, _context, _window);
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] eglMakeCurrent failed res=%d , error=0x%04X .\n",
               __FUNCTION__, __LINE__, res, eglGetError());
    return EGL_FALSE;
  }
  return EGL_TRUE;
}

EGLBoolean EglContext::makeCurrent(EGLSurface drawSurface,
                                   EGLSurface readSurface) {
  if (EGL_NO_DISPLAY == _display) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] no display .\n", __FUNCTION__,
               __LINE__);
    return EGL_FALSE;
  }
  if (EGL_NO_CONTEXT == _context) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] no context .\n", __FUNCTION__,
               __LINE__);
    return EGL_FALSE;
  }

  EGLBoolean res = eglMakeCurrent(_display, drawSurface, readSurface, _context);
  if (!res) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] eglMakeCurrent failed .\n",
               __FUNCTION__, __LINE__);
    return EGL_FALSE;
  }
  return EGL_TRUE;
}

EGLBoolean EglContext::setSurfaceSizeWithWindow() {
  if (!_EGLIsValid()) {
    return EGL_FALSE;
  }

#ifdef __HARMONY__
  {
    EGLint native_visual_id = 0;
    if (!eglGetConfigAttrib(_display, _config, EGL_NATIVE_VISUAL_ID,
                            &native_visual_id)) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] [EGL] eglGetConfigAttrib returned error=0x%04X .\n",
                 __FUNCTION__, __LINE__, eglGetError());
      EGLTerminate();
      return EGL_FALSE;
    }

    int width;
    int height;
    int ret1 = OH_NativeWindow_NativeWindowHandleOpt(
        _window, GET_BUFFER_GEOMETRY, &width, &height);
    int ret2 = OH_NativeWindow_NativeWindowHandleOpt(
        _window, SET_BUFFER_GEOMETRY, width, height);
    int ret3 = OH_NativeWindow_NativeWindowHandleOpt(_window, SET_FORMAT,
                                                     native_visual_id);
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] [EGL] ANativeWindow_setBuffersGeometry(f=%d) .\n",
               __FUNCTION__, __LINE__, native_visual_id);
    if (ret1 || ret2 || ret3) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] [EGL] ANativeWindow_setBuffersGeometry(format) "
                 "returned error %d %d %d.\n",
                 __FUNCTION__, __LINE__, ret1, ret2, ret3);
      EGLTerminate();
      return EGL_FALSE;
    }
    _width = getSurfaceWidth();
    _height = getSurfaceHeight();
    return (_width && _height) ? EGL_TRUE : EGL_FALSE;
  }
#endif
}

EGLBoolean EglContext::setSurfaceSize(int width, int height) {
  std::unique_lock<std::mutex> lck(_eglMutex);
  if (!_EGLIsValid()) {
    return EGL_FALSE;
  }

  _width = getSurfaceWidth();
  _height = getSurfaceHeight();

  if (width != _width || height != _height) {
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] ANativeWindow_setBuffersGeometry(w=%d,h=%d) -> "
               "(w=%d,h=%d) .\n",
               __FUNCTION__, __LINE__, _width, _height, width, height);
    int format;
    int ret1 =
        OH_NativeWindow_NativeWindowHandleOpt(_window, GET_FORMAT, &format);
    int ret2 = OH_NativeWindow_NativeWindowHandleOpt(
        _window, SET_BUFFER_GEOMETRY, width, height);
    int ret3 =
        OH_NativeWindow_NativeWindowHandleOpt(_window, SET_FORMAT, format);
    if (ret1 || ret2 || ret3) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] ANativeWindow_setBuffersGeometry error:%d %d %d.\n",
                 __FUNCTION__, __LINE__, ret1, ret2, ret3);
      return EGL_FALSE;
    }

    _width = getSurfaceWidth();
    _height = getSurfaceHeight();
    return (_width && _height) ? EGL_TRUE : EGL_FALSE;
  }

  return EGL_TRUE;
}

int EglContext::getSurfaceWidth() {
  EGLint width = 0;
  if (!eglQuerySurface(_display, _surface, EGL_WIDTH, &width)) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] eglQuerySurface error=0x%04X .\n",
               __FUNCTION__, __LINE__, eglGetError());
    return 0;
  }

  return width;
}

int EglContext::getSurfaceHeight() {
  EGLint height = 0;
  if (!eglQuerySurface(_display, _surface, EGL_HEIGHT, &height)) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] eglQuerySurface error=0x%04X .\n",
               __FUNCTION__, __LINE__, eglGetError());
    return 0;
  }

  return height;
}

EGLBoolean EglContext::swapBuffers() {
  if (eglGetCurrentContext() == EGL_NO_CONTEXT ||
      eglGetCurrentSurface(EGL_DRAW) == EGL_NO_SURFACE ||
      eglGetCurrentDisplay() == EGL_NO_DISPLAY) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d]  failed, error=0x%04X .\n",
               __FUNCTION__, __LINE__, eglGetError());
    return EGL_FALSE;
  }

  EGLBoolean res = makeCurrent();
  if (!res) {
    return EGL_FALSE;
  }

  res = eglSwapBuffers(_display, _surface);
  if (!res) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] eglSwapBuffers _display:%p, _surface:%p, _context:%p, "
               "_window:%p.\n",
               __FUNCTION__, __LINE__, _display, _surface, _context, _window);
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] eglSwapBuffers failed res=%d , error=0x%04X .\n",
               __FUNCTION__, __LINE__, res, eglGetError());
    return EGL_FALSE;
  }

  return EGL_TRUE;
}

int EglContext::_querySurface(int what) {
  int value;
  eglQuerySurface(_display, _surface, what, &value);
  return value;
}

EGLBoolean EglContext::_EGLIsValid() {
  if (_window && _display && _surface && _context) {
    return EGL_TRUE;
  }
  return EGL_FALSE;
}

EGLConfig EglContext::_getEGLConfig(int version) {
  int renderableType = EGL_OPENGL_ES2_BIT;
  if (version >= 3) {
    renderableType |= EGL_OPENGL_ES3_BIT_KHR;
  }
  EGLint configAttribs[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, renderableType,
      EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
      // EGL_DEPTH_SIZE, 16,
      // EGL_STENCIL_SIZE, 8,
      EGL_NONE};

  EGLConfig config = nullptr;
  EGLint numConfig;
  if (!eglChooseConfig(_display, configAttribs, &config, 1, &numConfig)) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] [EGL] eglChooseConfig failed error:0x%04X .\n",
               __FUNCTION__, __LINE__, eglGetError());
    return nullptr;
  }
  return config;
}
#endif

NS_REDRENDER_END
