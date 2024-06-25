#pragma once

#include <mutex>

#include "../../video_inc_internal.h"
#include "./gl_program.h"

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include "../android/egl_context.h"

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include "../harmony/egl_context.h"

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#include "../ios/eagl_context.h"

#endif

NS_REDRENDER_BEGIN

// opengl context class
class GLProgram;
class Context {
public:
  Context(const int &sessionID = 0);
  ~Context();

  static std::shared_ptr<Context> creatInstance(const int &sessionID = 0);

#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  std::shared_ptr<EglContext> getEglContext() const;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  std::shared_ptr<EaglContext> getEaglContext() const;
#endif
  void setActiveShaderProgram(GLProgram *shaderProgram);
  void clear();
  void opengl_print_string(const char *name, GLenum string);

protected:
  const int _sessionID{0};

private:
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  std::shared_ptr<EglContext> _eglContext;
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  std::shared_ptr<EaglContext> _eaglContext;
#endif
  GLProgram *_curShaderProgram;
};

NS_REDRENDER_END
