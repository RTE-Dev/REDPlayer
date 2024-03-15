/*
 * opengl_filter_base.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef OPENGL_FILTER_BASE_H
#define OPENGL_FILTER_BASE_H

#include "../../../common/redrender_buffer.h"
#include "../../video_filter.h"
#include "../../video_filter_info.h"
#include "../../video_inc_internal.h"
#include "../common/context.h"
#include "../common/gl_program.h"
#include "../common/gl_util.h"
#include "../input/opengl_video_input.h"
#include "../output/opengl_video_output.h"
#include "filter_shader.h"
#include <string>

NS_REDRENDER_BEGIN

typedef struct BackgroundColor {
  float r;
  float g;
  float b;
  float a;
} BackgroundColor;

typedef enum CropDirection {
  CropTop = 0,
  CropBottom,
  CropLeft,
  CropRight,
} CropDirection;

// opengl filter base class
class OpenGLFilterBase : public OpenGLVideoInput,
                         public OpenGLVideoOutput,
                         public VideoFilter {
public:
  OpenGLFilterBase(std::string openglFilterName = "OpenGLFilterBase",
                   VideoFilterType openglFilterType = VideoFilterTypeUnknown,
                   const int &sessionID = 0);
  virtual ~OpenGLFilterBase();
  //    static std::shared_ptr<OpenGLFilterBase> create();
  virtual VRError init(VideoFrameMetaData *inputFrameMetaData = nullptr,
                       std::shared_ptr<RedRender::Context> instance = nullptr);
  virtual bool
  initWithFragmentShaderString(const std::string &fragmentShaderSource,
                               int inputNumber = 1);
  bool initWithShaderString(const std::string &vertexShaderSource,
                            const std::string &fragmentShaderSource);

  virtual VRError onRender();
  virtual void updateParam(float frameTime);

  virtual std::string getOpenGLFilterBaseName();
  virtual VideoFilterType getOpenGLFilterBaseType();
  virtual void setFilterPriority(int priority);
  virtual int getFilterPriority();
  void setInputFrameMetaData(VideoFrameMetaData *inputFrameMetaData);

public:
  int pixelFormat;

protected:
  std::shared_ptr<RedRender::Context> mInstance;

  std::string _openglFilterName = "";
  VideoFilterType _openglFilterType;

  GLProgram *_filterProgram;
  GLuint _filterPositionAttribute;
  BackgroundColor _backgroundColor;

  std::string _getVertexShaderString() const;
  const GLfloat *_getVerticsCoordinate();
  const GLfloat *_getTexureCoordinate(const RotationMode &rotationMode);

  GLfloat _verticesCoordinate[8];
  void _verticesCoordinateReset();
  GLfloat _texureCoordinate[8];
  void _texureCoordinateReset(const RotationMode &rotationMode);
  void _texureCoordinateCrop(RotationMode rotationMode, GLfloat cropSize);
  int _filterPriority;
  VideoFrameMetaData *_inputFrameMetaData;
  GLsizei _paddingPixels{0}; // Used to crop redundant textures
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* OPENGL_FILTER_BASE_H */
