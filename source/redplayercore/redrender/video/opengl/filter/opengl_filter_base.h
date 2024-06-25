#pragma once

#include <string>

#include "../../../common/redrender_buffer.h"
#include "../../video_filter.h"
#include "../../video_inc_internal.h"
#include "../common/context.h"
#include "../common/gl_program.h"
#include "../common/gl_util.h"
#include "../input/opengl_video_input.h"
#include "../output/opengl_video_output.h"
#include "./filter_shader.h"

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
  OpenGLFilterBase(std::string openglFilterName,
                   VideoFilterType openglFilterType, const int &sessionID);
  virtual ~OpenGLFilterBase();
  virtual VRError init(VideoFrameMetaData *inputFrameMetaData,
                       std::shared_ptr<RedRender::Context> instance);
  virtual bool
  initWithFragmentShaderString(const std::string &fragmentShaderSource,
                               int inputNumber = 1);
  bool initWithShaderString(const std::string &vertexShaderSource,
                            const std::string &fragmentShaderSource);

  virtual VRError onRender();
  virtual void updateParam();

  virtual std::string getOpenGLFilterBaseName();
  virtual VideoFilterType getOpenGLFilterBaseType();
  virtual void setFilterPriority(int priority);
  virtual int getFilterPriority();
  void setInputFrameMetaData(VideoFrameMetaData *inputFrameMetaData);

public:
  int pixelFormat{0};

protected:
  std::shared_ptr<RedRender::Context> mInstance;

  std::string _openglFilterName = "";
  VideoFilterType _openglFilterType;

  GLProgram *_filterProgram{nullptr};
  GLuint _filterPositionAttribute;
  BackgroundColor _backgroundColor{0.0, 0.0, 0.0, 1.0};

  std::string _getVertexShaderString() const;
  const GLfloat *_getVerticsCoordinate();
  const GLfloat *_getTexureCoordinate(const RotationMode &rotationMode);

  GLfloat _verticesCoordinate[8];
  void _verticesCoordinateReset();
  GLfloat _texureCoordinate[8];
  void _texureCoordinateReset(const RotationMode &rotationMode);
  void _texureCoordinateCrop(RotationMode rotationMode, GLfloat cropSize);
  int _filterPriority{0};
  VideoFrameMetaData *_inputFrameMetaData{nullptr};
  GLsizei _paddingPixels{0}; // Used to crop redundant textures

protected:
  const int _sessionID{0};
};

NS_REDRENDER_END
