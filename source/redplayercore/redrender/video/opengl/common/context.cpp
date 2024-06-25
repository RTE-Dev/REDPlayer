/*
 * context.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./context.h"

NS_REDRENDER_BEGIN

Context::Context(const int &sessionID /* = 0 */)
    : _sessionID(sessionID), _curShaderProgram(nullptr) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  _eglContext = std::make_shared<EglContext>(sessionID);
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  _eaglContext = std::make_shared<EaglContext>(sessionID);
#endif
}

Context::~Context() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);

#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  if (_eglContext) {
    _eglContext.reset();
  }

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  if (_eaglContext) {
    _eaglContext.reset();
  }
#endif
}

std::shared_ptr<Context>
Context::creatInstance(const int &sessionID /* = 0 */) {
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::shared_ptr<Context> instance = std::make_shared<Context>(sessionID);
  return instance;
}

#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
std::shared_ptr<EglContext> Context::getEglContext() const {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _eglContext;
}
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
std::shared_ptr<EaglContext> Context::getEaglContext() const {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _eaglContext;
}
#endif

void Context::setActiveShaderProgram(GLProgram *shaderProgram) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (_curShaderProgram != shaderProgram) {
    _curShaderProgram = shaderProgram;
    shaderProgram->useGLProgram();
  }
}

void Context::clear() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

void Context::opengl_print_string(const char *name, GLenum string) {
  const char *version = (const char *)glGetString(string);
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] %s = %s .\n", __FUNCTION__, __LINE__,
             name, version);
}

NS_REDRENDER_END
