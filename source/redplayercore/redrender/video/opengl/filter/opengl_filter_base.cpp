/*
 * opengl_filter_base.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./opengl_filter_base.h"

#include <chrono>
#include <math.h>

#include "../common/context.h"

NS_REDRENDER_BEGIN

OpenGLFilterBase::OpenGLFilterBase(std::string openglFilterName,
                                   VideoFilterType openglFilterType,
                                   const int &sessionID)
    : OpenGLVideoInput(1, sessionID), OpenGLVideoOutput(sessionID),
      VideoFilter(sessionID), _openglFilterName(openglFilterName),
      _openglFilterType(openglFilterType), _sessionID(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

OpenGLFilterBase::~OpenGLFilterBase() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (_filterProgram) {
    delete _filterProgram;
    _filterProgram = nullptr;
  }
  if (mInstance) {
    mInstance.reset();
  }
}

VRError OpenGLFilterBase::init(VideoFrameMetaData *inputFrameMetaData,
                               std::shared_ptr<RedRender::Context> instance) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return VRError::VRErrorNone;
}

bool OpenGLFilterBase::initWithFragmentShaderString(
    const std::string &fragmentShaderSource, int inputTexNum) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _inputTexNum = inputTexNum;
  return initWithShaderString(_getVertexShaderString(), fragmentShaderSource);
}

bool OpenGLFilterBase::initWithShaderString(
    const std::string &vertexShaderSource,
    const std::string &fragmentShaderSource) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _filterProgram = GLProgram::createByShaderString(
      vertexShaderSource, fragmentShaderSource, _sessionID, mInstance);
  if (nullptr == _filterProgram) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] createByShaderString error.\n",
               __FUNCTION__, __LINE__);
    return false;
  }
  mInstance->setActiveShaderProgram(_filterProgram);
  _filterPositionAttribute = _filterProgram->getAttribLocation("position");
  CHECK_OPENGL(glEnableVertexAttribArray(_filterPositionAttribute));
  return true;
}

VRError OpenGLFilterBase::onRender() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  EGLBoolean res = mInstance->getEglContext()->makeCurrent();
  if (!res) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] makeCurrent error .\n",
               __FUNCTION__, __LINE__);
    return VRErrorOnRender;
  }
#endif
  mInstance->setActiveShaderProgram(_filterProgram);
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  _framebuffer->setViewPort();
  if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
    mInstance->getEglContext()->setSurfaceSize(
        _framebuffer->getWidth(),
        _framebuffer
            ->getHeight()); // the last device filter direct screen filter.
  } else {
    _framebuffer->activeFramebuffer();
  }
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  mInstance->getEaglContext()->useAsCurrent();
  if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
    mInstance->getEaglContext()->activeFramebuffer();
  } else {
    _framebuffer->setViewPort();
    _framebuffer->activeFramebuffer();
  }
#endif
  CHECK_OPENGL(glClearColor(_backgroundColor.r, _backgroundColor.g,
                            _backgroundColor.b, _backgroundColor.a));
  CHECK_OPENGL(glClear(GL_COLOR_BUFFER_BIT));
  for (std::map<int, InputFrameBufferInfo>::const_iterator iter =
           _inputFramebuffers.begin();
       iter != _inputFramebuffers.end(); ++iter) {
    int texIdx = iter->first;
    std::shared_ptr<Framebuffer> fb = iter->second.frameBuffer;
    CHECK_OPENGL(glActiveTexture(GL_TEXTURE0 + texIdx));
    CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, fb->getTexture()));
    _filterProgram->setUniformValue(
        texIdx == 0 ? "colorMap" : strFormat("colorMap%d", texIdx), texIdx);
    // texcoord attribute
    GLuint filterTexCoordAttribute = _filterProgram->getAttribLocation(
        texIdx == 0 ? "texCoord" : strFormat("texCoord%d", texIdx));
    CHECK_OPENGL(glEnableVertexAttribArray(filterTexCoordAttribute));
    CHECK_OPENGL(glVertexAttribPointer(
        filterTexCoordAttribute, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
        _getTexureCoordinate(iter->second.rotationMode)));
  }
  GLuint um4_mvp = _filterProgram->getUniformLocation("um4_mvp");
  _filterProgram->setUniformValue4(
      um4_mvp, const_cast<GLfloat *>(getOpenGLModelViewProjectionMatrix()));
  CHECK_OPENGL(glVertexAttribPointer(_filterPositionAttribute, 2, GL_FLOAT,
                                     GL_FALSE, 2 * sizeof(float),
                                     _getVerticsCoordinate()));
  CHECK_OPENGL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
    mInstance->getEglContext()
        ->swapBuffers(); // the last device filter direct screen filter.
  } else {
    _framebuffer->inActiveFramebuffer();
  }
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
    mInstance->getEaglContext()->presentBufferForDisplay();
    mInstance->getEaglContext()->inActiveFramebuffer();
  } else {
    _framebuffer->inActiveFramebuffer();
  }
  mInstance->getEaglContext()->useAsPrev();
#endif
  return VRError::VRErrorNone;
}

void OpenGLFilterBase::updateParam() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (_inputFramebuffers.empty()) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] _inputFramebuffers empty error .\n", __FUNCTION__,
               __LINE__);
    return;
  }
  std::shared_ptr<Framebuffer> firstInputFramebuffer =
      _inputFramebuffers.begin()->second.frameBuffer;
  RotationMode firstInputRotation =
      _inputFramebuffers.begin()->second.rotationMode;
  if (!firstInputFramebuffer) {
    return;
  }

  int rotatedFramebufferWidth = firstInputFramebuffer->getWidth();
  int rotatedFramebufferHeight = firstInputFramebuffer->getHeight();
  if (rotationSwapsSize(firstInputRotation)) {
    rotatedFramebufferWidth = firstInputFramebuffer->getHeight();
    rotatedFramebufferHeight = firstInputFramebuffer->getWidth();
    if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
      _inputFrameMetaData->frameWidth = rotatedFramebufferWidth;
      _inputFrameMetaData->frameHeight = rotatedFramebufferHeight;
    }
  }
  if (_framebufferScale != 1.0) {
    rotatedFramebufferWidth =
        static_cast<int>(rotatedFramebufferWidth * _framebufferScale);
    rotatedFramebufferHeight =
        static_cast<int>(rotatedFramebufferHeight * _framebufferScale);
    if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
      _inputFrameMetaData->frameWidth = rotatedFramebufferWidth;
      _inputFrameMetaData->frameHeight = rotatedFramebufferHeight;
    }
  }
  if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
    if (_inputFrameMetaData->linesize[0] > _inputFrameMetaData->frameWidth) {
      _paddingPixels =
          _inputFrameMetaData->linesize[0] - _inputFrameMetaData->frameWidth;
    }
  }

  bool onlyTexture = false;
  if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType) {
    onlyTexture = true; // the last direct screen filter.
  }
  if (nullptr == _framebuffer ||
      _framebuffer->getWidth() != rotatedFramebufferWidth ||
      _framebuffer->getHeight() != rotatedFramebufferHeight) {
    if (_framebuffer) {
      _framebuffer.reset();
    }
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
    _framebuffer = Framebuffer::create(
        rotatedFramebufferWidth, rotatedFramebufferHeight, onlyTexture,
        Framebuffer::defaultTextureAttribures, _sessionID);
#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
    if (getOpenGLFilterBaseType() != VideoOpenGLDeviceFilterType) {
      _framebuffer = Framebuffer::create(
          rotatedFramebufferWidth, rotatedFramebufferHeight, onlyTexture,
          Framebuffer::defaultTextureAttribures, _sessionID);
    }
#endif
  }
}

std::string OpenGLFilterBase::getOpenGLFilterBaseName() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _openglFilterName;
}

VideoFilterType OpenGLFilterBase::getOpenGLFilterBaseType() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _openglFilterType;
}

void OpenGLFilterBase::setFilterPriority(int priority) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _filterPriority = priority;
}

int OpenGLFilterBase::getFilterPriority() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);

  return _filterPriority;
}

void OpenGLFilterBase::setInputFrameMetaData(
    VideoFrameMetaData *inputFrameMetaData) {
  _inputFrameMetaData = inputFrameMetaData;
}

std::string OpenGLFilterBase::_getVertexShaderString() const {
  if (_inputTexNum <= 1) {
    return getOpenGLDeviceFilterVertexShaderString();
  }

  std::string shaderStr = "\
            precision highp float;\n\
            attribute highp vec4 position;\n\
            attribute highp vec4 texCoord;\n\
            varying highp vec2 vTexCoord;\n\
            uniform mat4 um4_mvp;\n\
            ";
  for (int i = 1; i < _inputTexNum; ++i) {
    shaderStr += strFormat("\
            attribute highp vec4 texCoord%d;\n\
            varying highp vec2 vTexCoord%d;\n\
                            ",
                           i, i);
  }
  shaderStr += "\
            void main()\n\
            {\n\
                gl_Position = um4_mvp * position;\n\
                vTexCoord = texCoord.xy;\n\
    ";
  for (int i = 1; i < _inputTexNum; ++i) {
    shaderStr += strFormat("vTexCoord%d = texCoord%d.xy;\n", i, i);
  }
  shaderStr += "}\n";

  return shaderStr;
}

const GLfloat *OpenGLFilterBase::_getVerticsCoordinate() {
  _verticesCoordinateReset();
  if (_openglFilterType != VideoOpenGLDeviceFilterType) {
    return _verticesCoordinate;
  }

  if (_inputFrameMetaData->layerWidth < 0 ||
      _inputFrameMetaData->layerHeight < 0 ||
      _inputFrameMetaData->frameWidth < 0 ||
      _inputFrameMetaData->frameHeight < 0) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] invalid width/height for gravity aspect .\n",
               __FUNCTION__, __LINE__);
    return _verticesCoordinate;
  }

  float frameWidth = _inputFrameMetaData->frameWidth;
  float frameHeight = _inputFrameMetaData->frameHeight;
  float x_Value = 1.0f;
  float y_Value = 1.0f;

  switch (_inputFrameMetaData->aspectRatio) {
  case AspectRatioEqualWidth: {
    if (_inputFrameMetaData->sar.num > 0 && _inputFrameMetaData->sar.den > 0) {
      frameHeight = frameHeight * _inputFrameMetaData->sar.den /
                    _inputFrameMetaData->sar.num;
    }
    y_Value =
        ((frameHeight * 1.0f / frameWidth) * _inputFrameMetaData->layerWidth /
         _inputFrameMetaData->layerHeight);
    _verticesCoordinate[1] = -1.0f * y_Value;
    _verticesCoordinate[3] = -1.0f * y_Value;
    _verticesCoordinate[5] = 1.0f * y_Value;
    _verticesCoordinate[7] = 1.0f * y_Value;
  } break;
  case AspectRatioEqualHeight: {
    if (_inputFrameMetaData->sar.num > 0 && _inputFrameMetaData->sar.den > 0) {
      frameWidth = frameWidth * _inputFrameMetaData->sar.num /
                   _inputFrameMetaData->sar.den;
    }
    x_Value =
        ((frameWidth * 1.0f / frameHeight) * _inputFrameMetaData->layerHeight /
         _inputFrameMetaData->layerWidth);
    _verticesCoordinate[0] = -1.0f * x_Value;
    _verticesCoordinate[2] = 1.0f * x_Value;
    _verticesCoordinate[4] = -1.0f * x_Value;
    _verticesCoordinate[6] = 1.0f * x_Value;
  } break;
  case RedRender::ScaleAspectFit: {
    if (_inputFrameMetaData->sar.num > 0 && _inputFrameMetaData->sar.den > 0) {
      frameWidth = frameWidth * _inputFrameMetaData->sar.num /
                   _inputFrameMetaData->sar.den;
    }
    const float dW = static_cast<float>(_inputFrameMetaData->layerWidth) /
                     _inputFrameMetaData->frameWidth;
    const float dH = static_cast<float>(_inputFrameMetaData->layerHeight) /
                     _inputFrameMetaData->frameHeight;

    float dd = 1.0f;
    float nW = 1.0f;
    float nH = 1.0f;
    dd = fmin(dW, dH);

    nW = (_inputFrameMetaData->frameWidth * dd /
          static_cast<float>(_inputFrameMetaData->layerWidth));
    nH = (_inputFrameMetaData->frameHeight * dd /
          static_cast<float>(_inputFrameMetaData->layerHeight));

    _verticesCoordinate[0] = -nW;
    _verticesCoordinate[1] = -nH;
    _verticesCoordinate[2] = nW;
    _verticesCoordinate[3] = -nH;
    _verticesCoordinate[4] = -nW;
    _verticesCoordinate[5] = nH;
    _verticesCoordinate[6] = nW;
    _verticesCoordinate[7] = nH;
  } break;
  case RedRender::ScaleAspectFill: {
    if (_inputFrameMetaData->sar.num > 0 && _inputFrameMetaData->sar.den > 0) {
      frameWidth = frameWidth * _inputFrameMetaData->sar.num /
                   _inputFrameMetaData->sar.den;
    }
    const float dW = static_cast<float>(_inputFrameMetaData->layerWidth) /
                     _inputFrameMetaData->frameWidth;
    const float dH = static_cast<float>(_inputFrameMetaData->layerHeight) /
                     _inputFrameMetaData->frameHeight;

    float dd = 1.0f;
    float nW = 1.0f;
    float nH = 1.0f;
    dd = fmax(dW, dH);

    nW = (_inputFrameMetaData->frameWidth * dd /
          static_cast<float>(_inputFrameMetaData->layerWidth));
    nH = (_inputFrameMetaData->frameHeight * dd /
          static_cast<float>(_inputFrameMetaData->layerHeight));

    _verticesCoordinate[0] = -nW;
    _verticesCoordinate[1] = -nH;
    _verticesCoordinate[2] = nW;
    _verticesCoordinate[3] = -nH;
    _verticesCoordinate[4] = -nW;
    _verticesCoordinate[5] = nH;
    _verticesCoordinate[6] = nW;
    _verticesCoordinate[7] = nH;
  } break;
  case AspectRatioFill:
  default:
    AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] AspectRatioFill .\n", __FUNCTION__,
               __LINE__);
    break;
  }
  return _verticesCoordinate;
}

const GLfloat *
OpenGLFilterBase::_getTexureCoordinate(const RotationMode &rotationMode) {
  _texureCoordinateReset(rotationMode);
  if (_paddingPixels > 0) {
    GLfloat cropSize =
        ((GLfloat)_paddingPixels) / _inputFrameMetaData->linesize[0];
    _texureCoordinateCrop(rotationMode, cropSize);
  }
  return _texureCoordinate;
}

void OpenGLFilterBase::_texureCoordinateReset(
    const RotationMode &rotationMode) {
  switch (rotationMode) {
  case RotateLeft:
    _texureCoordinate[0] = 0.0f;
    _texureCoordinate[1] = 0.0f;
    _texureCoordinate[2] = 0.0f;
    _texureCoordinate[3] = 1.0f;
    _texureCoordinate[4] = 1.0f;
    _texureCoordinate[5] = 0.0f;
    _texureCoordinate[6] = 1.0f;
    _texureCoordinate[7] = 1.0f;
    break;
  case RotateRight:
    _texureCoordinate[0] = 1.0f;
    _texureCoordinate[1] = 1.0f;
    _texureCoordinate[2] = 1.0f;
    _texureCoordinate[3] = 0.0f;
    _texureCoordinate[4] = 0.0f;
    _texureCoordinate[5] = 1.0f;
    _texureCoordinate[6] = 0.0f;
    _texureCoordinate[7] = 0.0f;
    break;
  case FlipVertical:
    _texureCoordinate[0] = 0.0f;
    _texureCoordinate[1] = 0.0f;
    _texureCoordinate[2] = 1.0f;
    _texureCoordinate[3] = 0.0f;
    _texureCoordinate[4] = 0.0f;
    _texureCoordinate[5] = 1.0f;
    _texureCoordinate[6] = 1.0f;
    _texureCoordinate[7] = 1.0f;
    break;
  case FlipHorizontal:
    _texureCoordinate[0] = 1.0f;
    _texureCoordinate[1] = 1.0f;
    _texureCoordinate[2] = 0.0f;
    _texureCoordinate[3] = 1.0f;
    _texureCoordinate[4] = 1.0f;
    _texureCoordinate[5] = 0.0f;
    _texureCoordinate[6] = 0.0f;
    _texureCoordinate[7] = 0.0f;
    break;
  case RotateRightFlipVertical:
    _texureCoordinate[0] = 1.0f;
    _texureCoordinate[1] = 0.0f;
    _texureCoordinate[2] = 1.0f;
    _texureCoordinate[3] = 1.0f;
    _texureCoordinate[4] = 0.0f;
    _texureCoordinate[5] = 0.0f;
    _texureCoordinate[6] = 0.0f;
    _texureCoordinate[7] = 1.0f;
    break;
  case RotateRightFlipHorizontal:
    _texureCoordinate[0] = 0.0f;
    _texureCoordinate[1] = 1.0f;
    _texureCoordinate[2] = 0.0f;
    _texureCoordinate[3] = 0.0f;
    _texureCoordinate[4] = 1.0f;
    _texureCoordinate[5] = 1.0f;
    _texureCoordinate[6] = 1.0f;
    _texureCoordinate[7] = 0.0f;
    break;
  case Rotate180:
    _texureCoordinate[0] = 1.0f;
    _texureCoordinate[1] = 0.0f;
    _texureCoordinate[2] = 0.0f;
    _texureCoordinate[3] = 0.0f;
    _texureCoordinate[4] = 1.0f;
    _texureCoordinate[5] = 1.0f;
    _texureCoordinate[6] = 0.0f;
    _texureCoordinate[7] = 1.0f;
    break;
  case NoRotation:
  default:
    _texureCoordinate[0] = 0.0f;
    _texureCoordinate[1] = 1.0f;
    _texureCoordinate[2] = 1.0f;
    _texureCoordinate[3] = 1.0f;
    _texureCoordinate[4] = 0.0f;
    _texureCoordinate[5] = 0.0f;
    _texureCoordinate[6] = 1.0f;
    _texureCoordinate[7] = 0.0f;
    break;
  }
}

void OpenGLFilterBase::_verticesCoordinateReset() {
  _verticesCoordinate[0] = -1.0f;
  _verticesCoordinate[1] = -1.0f;
  _verticesCoordinate[2] = 1.0f;
  _verticesCoordinate[3] = -1.0f;
  _verticesCoordinate[4] = -1.0f;
  _verticesCoordinate[5] = 1.0f;
  _verticesCoordinate[6] = 1.0f;
  _verticesCoordinate[7] = 1.0f;
}

void OpenGLFilterBase::_texureCoordinateCrop(RotationMode rotationMode,
                                             GLfloat cropSize) {
  switch (rotationMode) {
  case RotateLeft:
    _texureCoordinate[4] = _texureCoordinate[4] - cropSize;
    _texureCoordinate[6] = _texureCoordinate[6] - cropSize;
    break;
  case RotateRight:
    _texureCoordinate[0] = _texureCoordinate[0] - cropSize;
    _texureCoordinate[2] = _texureCoordinate[2] - cropSize;
    break;
  case FlipVertical:
    _texureCoordinate[2] = _texureCoordinate[2] - cropSize;
    _texureCoordinate[6] = _texureCoordinate[6] - cropSize;
    break;
  case FlipHorizontal:
    _texureCoordinate[0] = _texureCoordinate[0] - cropSize;
    _texureCoordinate[4] = _texureCoordinate[4] - cropSize;
    break;
  case RotateRightFlipVertical:
    _texureCoordinate[0] = _texureCoordinate[0] - cropSize;
    _texureCoordinate[2] = _texureCoordinate[2] - cropSize;
    break;
  case RotateRightFlipHorizontal:
    _texureCoordinate[4] = _texureCoordinate[4] - cropSize;
    _texureCoordinate[6] = _texureCoordinate[6] - cropSize;
    break;
  case Rotate180:
    _texureCoordinate[0] = _texureCoordinate[0] - cropSize;
    _texureCoordinate[4] = _texureCoordinate[4] - cropSize;
    break;
  case NoRotation:
  default:
    _texureCoordinate[2] = _texureCoordinate[2] - cropSize;
    if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType &&
        _texureCoordinate[2] > 0 && _texureCoordinate[6] > 0) {
      uint64_t tenThousands = 0;
      tenThousands = (uint64_t)(floorf(_texureCoordinate[2] * 10000)) % 10;
      if (tenThousands > 5) {
        _texureCoordinate[2] = floorf(_texureCoordinate[2] * 1000.0) / 1000.0;
      } else {
        _texureCoordinate[2] =
            floorf(_texureCoordinate[2] * 1000.0) / 1000.0 - 0.001;
      }
    }

    _texureCoordinate[6] = _texureCoordinate[6] - cropSize;
    if (getOpenGLFilterBaseType() == VideoOpenGLDeviceFilterType &&
        _texureCoordinate[2] > 0 && _texureCoordinate[6] > 0) {
      uint64_t tenThousands = 0;
      tenThousands = (uint64_t)(floorf(_texureCoordinate[6] * 10000)) % 10;
      if (tenThousands > 5) {
        _texureCoordinate[6] = floorf(_texureCoordinate[6] * 1000.0) / 1000.0;
      } else {
        _texureCoordinate[6] =
            floorf(_texureCoordinate[6] * 1000.0) / 1000.0 - 0.001;
      }
    }
    break;
  }
}

NS_REDRENDER_END
