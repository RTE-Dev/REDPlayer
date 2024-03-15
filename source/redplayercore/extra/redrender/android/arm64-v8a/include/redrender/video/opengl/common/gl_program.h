/*
 * gl_program.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef GL_PROGRAM_H
#define GL_PROGRAM_H

#include "../../video_inc_internal.h"
#include "context.h"
#include <string>

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#endif

NS_REDRENDER_BEGIN

// opengl gl program class
class Context;
class GLProgram {
public:
  GLProgram(const int &sessionID = 0);
  ~GLProgram();

  static GLProgram *
  createByShaderString(const std::string &vertexShaderSource,
                       const std::string &fragmentShaderSource,
                       const int &sessionID = 0,
                       std::shared_ptr<RedRender::Context> instance = nullptr);
  void useGLProgram();
  GLuint getGLProgramID() const { return _program; }

  GLuint getAttribLocation(const std::string &attribute);
  GLuint getUniformLocation(const std::string &uniformName);

  void setUniformValue(const std::string &uniformName, int value);
  void setUniformValue(const std::string &uniformName, float value);
  void setUniformValue(const std::string &uniformName, GLfloat *value);

  void setUniformValue(int uniformLocation, int value);
  void setUniformValue(int uniformLocation, float value);
  void setUniformValue(int uniformLocation, GLfloat *value);
  void setUniformValue4(int uniformLocation, GLfloat *value);

private:
  void _printShaderInfo(GLuint shader);
  bool
  _initWithShaderString(const std::string &vertexShaderSource,
                        const std::string &fragmentShaderSource,
                        std::shared_ptr<RedRender::Context> instance = nullptr);

  std::shared_ptr<RedRender::Context> mInstance;
  GLuint _program;
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* GL_PROGRAM_H */
