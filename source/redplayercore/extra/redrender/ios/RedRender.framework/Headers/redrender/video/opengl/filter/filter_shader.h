/*
 * filter_shader.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef FILTER_SHADER_H
#define FILTER_SHADER_H

#include "../../video_inc_internal.h"
#include "opengl_filter_base.h"
#ifdef __APPLE__
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#endif

NS_REDRENDER_BEGIN
const std::string getOpenGLDeviceFilterFragmentShaderString();
const std::string getOpenGLDeviceFilterVertexShaderString();

const std::string getYUV420pVideoRange2RGBFilterFragmentShaderString();
const std::string getYUV420pFullRange2RGBFilterFragmentShaderString();
const std::string getYUV420spVideoRange2RGBFilterFragmentShaderString();
const std::string getYUV420spFullRange2RGBFilterFragmentShaderString();
const std::string getYUV444p10leVideoRange2RGBFilterFragmentShaderString();
const std::string getYUV444p10leFullRange2RGBFilterFragmentShaderString();

const std::string getMirrorFilterFragmentShaderString();
const std::string getBlackAndWhiteFilterFragmentShaderString();

const GLfloat *getBt601VideoRangeYUV2RGBMatrix();
const GLfloat *getBt601FullRangeYUV2RGBMatrix();
const GLfloat *getBt709VideoRangeYUV2RGBMatrix();
const GLfloat *getBt709FullRangeYUV2RGBMatrix();
const GLfloat *getBt2020VideoRangeYUV2RGBMatrix();
const GLfloat *getBt2020FullRangeYUV2RGBMatrix();

const GLfloat *getOpenGLModelViewProjectionMatrix();

NS_REDRENDER_END

#endif /* FILTER_SHADER_H */
