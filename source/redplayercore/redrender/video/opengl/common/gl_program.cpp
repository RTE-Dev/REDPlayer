/*
 * gl_program.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./gl_program.h"

NS_REDRENDER_BEGIN

GLProgram::GLProgram(const int &sessionID /* = 0 */)
    : _program(-1), _sessionID(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

GLProgram::~GLProgram() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (-1 != _program) {
    glDeleteProgram(_program);
    _program = -1;
  }
  if (mInstance) {
    mInstance.reset();
  }
}

GLProgram *
GLProgram::createByShaderString(const std::string &vertexShaderSource,
                                const std::string &fragmentShaderSource,
                                const int &sessionID /* = 0 */,
                                std::shared_ptr<RedRender::Context> instance) {
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  GLProgram *ptr = new (std::nothrow) GLProgram(sessionID);
  if (nullptr == ptr) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] new GLProgram() error .\n",
               __FUNCTION__, __LINE__);
    return nullptr;
  }
  if (!ptr->_initWithShaderString(vertexShaderSource, fragmentShaderSource,
                                  instance)) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] _initWithShaderString error .\n",
               __FUNCTION__, __LINE__);
    delete ptr;
    ptr = nullptr;
  }
  return ptr;
}

void GLProgram::_printShaderInfo(GLuint shader) {
  if (!shader) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] shader error , _program:%d .\n",
               __FUNCTION__, __LINE__, _program);
    return;
  }

  GLint infoLen = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
  if (!infoLen) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] [GLES2][Shader] empty info , _program:%d .\n",
               __FUNCTION__, __LINE__, _program);
    return;
  }

  char *bufInfo = new char[infoLen + 1];
  if (!bufInfo) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] error , _program:%d .\n",
               __FUNCTION__, __LINE__, _program);
    return;
  }
  glGetShaderInfoLog(shader, infoLen, nullptr, bufInfo);
  AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] [GLES2][Shader] error %s .\n",
             __FUNCTION__, __LINE__, bufInfo);
  if (bufInfo) {
    delete[] bufInfo;
  }
}

bool GLProgram::_initWithShaderString(
    const std::string &vertexShaderSource,
    const std::string &fragmentShaderSource,
    std::shared_ptr<RedRender::Context> instance) {
  if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] vertexShaderSource or fragmentShaderSource is empty.\n",
               __FUNCTION__, __LINE__);
    return false;
  }
  if (!instance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] instance = nullptr error.\n",
               __FUNCTION__, __LINE__);
    return false;
  }
  bool ret = true;
  GLint compileStatus = 0;
  GLint linkStatus = GL_FALSE;
  const char *vertexShaderSourceStr = vertexShaderSource.c_str();
  const char *fragmentShaderSourceStr = fragmentShaderSource.c_str();
  GLuint fragShader = 0;

  mInstance = instance;
  if (_program != -1) {
    CHECK_OPENGL(glDeleteProgram(_program));
    _program = -1;
  }
  CHECK_OPENGL(_program = glCreateProgram());
  CHECK_OPENGL(GLuint vertShader = glCreateShader(GL_VERTEX_SHADER));
  if (!vertShader) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] glCreateShader GL_VERTEX_SHADER error, _program:%d.\n",
               __FUNCTION__, __LINE__, _program);
    return GL_FALSE;
  }
  CHECK_OPENGL(glShaderSource(vertShader, 1, &vertexShaderSourceStr, NULL));
  CHECK_OPENGL(glCompileShader(vertShader));

  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compileStatus);
  if (!compileStatus) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] compile vertexShaderSourceStr:%s error, _program:%d.\n",
               __FUNCTION__, __LINE__, vertexShaderSourceStr, _program);
    ret = false;
    goto fail;
  }
  CHECK_OPENGL(fragShader = glCreateShader(GL_FRAGMENT_SHADER));
  if (!fragShader) {
    AV_LOGE_ID(
        LOG_TAG, _sessionID,
        "[%s:%d] glCreateShader GL_FRAGMENT_SHADER error, _program:%d.\n",
        __FUNCTION__, __LINE__, _program);
    ret = false;
    goto fail;
  }
  CHECK_OPENGL(glShaderSource(fragShader, 1, &fragmentShaderSourceStr, NULL));
  CHECK_OPENGL(glCompileShader(fragShader));
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compileStatus);
  if (!compileStatus) {
    AV_LOGE_ID(
        LOG_TAG, _sessionID,
        "[%s:%d] compile fragmentShaderSourceStr:%s, _program:%d error.\n",
        __FUNCTION__, __LINE__, fragmentShaderSourceStr, _program);
    ret = false;
    goto fail;
  }

  CHECK_OPENGL(glAttachShader(_program, vertShader));
  CHECK_OPENGL(glAttachShader(_program, fragShader));
  CHECK_OPENGL(glLinkProgram(_program));
  glGetProgramiv(_program, GL_LINK_STATUS, &linkStatus);
  if (!linkStatus) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] glLinkProgram _program:%d error.\n", __FUNCTION__,
               __LINE__, _program);
    ret = false;
    goto fail;
  }

fail:
  if (vertShader) {
    CHECK_OPENGL(glDeleteShader(vertShader));
  }
  if (fragShader) {
    CHECK_OPENGL(glDeleteShader(fragShader));
  }

  return ret;
}

void GLProgram::useGLProgram() { CHECK_OPENGL(glUseProgram(_program)); }

GLuint GLProgram::getAttribLocation(const std::string &attribute) {
  return glGetAttribLocation(_program, attribute.c_str());
}

GLuint GLProgram::getUniformLocation(const std::string &uniformName) {
  return glGetUniformLocation(_program, uniformName.c_str());
}

void GLProgram::setUniformValue(const std::string &uniformName, int value) {
  mInstance->setActiveShaderProgram(this);
  setUniformValue(getUniformLocation(uniformName), value);
}

void GLProgram::setUniformValue(const std::string &uniformName, float value) {
  mInstance->setActiveShaderProgram(this);
  setUniformValue(getUniformLocation(uniformName), value);
}

void GLProgram::setUniformValue(const std::string &uniformName,
                                GLfloat *value) {
  mInstance->setActiveShaderProgram(this);
  setUniformValue(getUniformLocation(uniformName), value);
}

void GLProgram::setUniformValue(int uniformLocation, int value) {
  mInstance->setActiveShaderProgram(this);
  CHECK_OPENGL(glUniform1i(uniformLocation, value));
}

void GLProgram::setUniformValue(int uniformLocation, float value) {
  mInstance->setActiveShaderProgram(this);
  CHECK_OPENGL(glUniform1f(uniformLocation, value));
}

void GLProgram::setUniformValue(int uniformLocation, GLfloat *value) {
  mInstance->setActiveShaderProgram(this);
  CHECK_OPENGL(glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, value));
}

void GLProgram::setUniformValue4(int uniformLocation, GLfloat *value) {
  mInstance->setActiveShaderProgram(this);
  CHECK_OPENGL(glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, value));
}

NS_REDRENDER_END
