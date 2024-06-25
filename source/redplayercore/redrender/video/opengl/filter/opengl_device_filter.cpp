/*
 * opengl_device_filter.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./opengl_device_filter.h"

NS_REDRENDER_BEGIN

OpenGLDeviceFilter::OpenGLDeviceFilter(const int &sessionID)
    : OpenGLFilterBase("OpenGLDeviceFilter", VideoOpenGLDeviceFilterType,
                       sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

OpenGLDeviceFilter::~OpenGLDeviceFilter() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

std::shared_ptr<OpenGLFilterBase>
OpenGLDeviceFilter::create(VideoFrameMetaData *inputFrameMetaData,
                           const int &sessionID,
                           std::shared_ptr<RedRender::Context> instance) {
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::shared_ptr<OpenGLDeviceFilter> ptr =
      std::make_shared<OpenGLDeviceFilter>(sessionID);

  VRError ret = ptr->init(inputFrameMetaData, instance);
  if (ret != VRErrorNone) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] OpenGLDeviceFilter init error .\n",
               __FUNCTION__, __LINE__);
    return nullptr;
  }
  AV_LOGI_ID(LOG_TAG, sessionID, "[%s:%d] filter=%s create success .\n",
             __FUNCTION__, __LINE__, ptr->getOpenGLFilterBaseName().c_str());

  return ptr;
}

VRError OpenGLDeviceFilter::init(VideoFrameMetaData *inputFrameMetaData,
                                 std::shared_ptr<RedRender::Context> instance) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (!instance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] instance = nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInitFilter;
  }
  mInstance = instance;
  _inputFrameMetaData = inputFrameMetaData;
  pixelFormat = inputFrameMetaData->pixel_format;
  switch (_inputFrameMetaData->pixel_format) {
  case VRPixelFormatRGB565:
  case VRPixelFormatRGB888:
  case VRPixelFormatRGBX8888: {
    if (!initWithFragmentShaderString(
            getOpenGLDeviceFilterFragmentShaderString(), 1)) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInitFilter;
    }
  } break;
  case VRPixelFormatYUV420p: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      if (!initWithFragmentShaderString(
              getYUV420pFullRange2RGBFilterFragmentShaderString(), 3)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    } else {
      if (!initWithFragmentShaderString(
              getYUV420pVideoRange2RGBFilterFragmentShaderString(), 3)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    }
  } break;
  case VRPixelFormatYUV420sp: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      if (!initWithFragmentShaderString(
              getYUV420spFullRange2RGBFilterFragmentShaderString(), 2)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    } else {
      if (!initWithFragmentShaderString(
              getYUV420spVideoRange2RGBFilterFragmentShaderString(), 2)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    }
  } break;
  case VRPixelFormatYUV420sp_vtb: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      if (!initWithFragmentShaderString(
              getYUV420spFullRange2RGBFilterFragmentShaderString(), 2)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    } else {
      if (!initWithFragmentShaderString(
              getYUV420spVideoRange2RGBFilterFragmentShaderString(), 2)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    }
  } break;
  case VRPixelFormatYUV420p10le: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      if (!initWithFragmentShaderString(
              getYUV444p10leFullRange2RGBFilterFragmentShaderString(), 3)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    } else {
      if (!initWithFragmentShaderString(
              getYUV444p10leVideoRange2RGBFilterFragmentShaderString(), 3)) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] init filter error .\n",
                   __FUNCTION__, __LINE__);
        return VRError::VRErrorInitFilter;
      }
    }
  } break;
  case VRPixelFormatMediaCodec:
    break;
  case VRPixelFormatHisiMediaCodec:
    break;
  case VRPixelFormatHarmonyVideoDecoder:
    break;
  default:
    break;
  }
  return VRError::VRErrorNone;
}

VRError OpenGLDeviceFilter::onRender() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  switch (_inputFrameMetaData->color_space) {
  case VR_AVCOL_SPC_RGB:
    break;
  case VR_AVCOL_SPC_BT709:
  case VR_AVCOL_SPC_SMPTE240M: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      _filterProgram->setUniformValue(
          "um3_ColorConversion",
          const_cast<GLfloat *>(getBt709FullRangeYUV2RGBMatrix()));
    } else {
      _filterProgram->setUniformValue(
          "um3_ColorConversion",
          const_cast<GLfloat *>(getBt709VideoRangeYUV2RGBMatrix()));
    }
    break;
  }
  case VR_AVCOL_SPC_BT470BG:
  case VR_AVCOL_SPC_SMPTE170M: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      _filterProgram->setUniformValue(
          "um3_ColorConversion",
          const_cast<GLfloat *>(getBt601FullRangeYUV2RGBMatrix()));
    } else {
      _filterProgram->setUniformValue(
          "um3_ColorConversion",
          const_cast<GLfloat *>(getBt601VideoRangeYUV2RGBMatrix()));
    }
  } break;
  case VR_AVCOL_SPC_BT2020_NCL:
  case VR_AVCOL_SPC_BT2020_CL: {
    if (_inputFrameMetaData->color_range == VR_AVCOL_RANGE_JPEG) {
      _filterProgram->setUniformValue(
          "um3_ColorConversion",
          const_cast<GLfloat *>(getBt2020FullRangeYUV2RGBMatrix()));
    } else {
      _filterProgram->setUniformValue(
          "um3_ColorConversion",
          const_cast<GLfloat *>(getBt2020VideoRangeYUV2RGBMatrix()));
    }
  } break;
  default: {
    if (_inputFrameMetaData->pixel_format == VRPixelFormatYUV420p ||
        _inputFrameMetaData->pixel_format == VRPixelFormatYUV420sp ||
        _inputFrameMetaData->pixel_format == VRPixelFormatYUV420sp_vtb ||
        _inputFrameMetaData->pixel_format == VRPixelFormatYUV420p10le) {
      if (_inputFrameMetaData->color_range ==
          VR_AVCOL_RANGE_JPEG) { // In case color_space=unknown playback
                                 // error
        _filterProgram->setUniformValue(
            "um3_ColorConversion",
            const_cast<GLfloat *>(getBt709FullRangeYUV2RGBMatrix()));
      } else {
        _filterProgram->setUniformValue(
            "um3_ColorConversion",
            const_cast<GLfloat *>(getBt709VideoRangeYUV2RGBMatrix()));
      }
    }
  } break;
  }
  return OpenGLFilterBase::onRender();
}

std::string OpenGLDeviceFilter::getOpenGLFilterBaseName() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _openglFilterName;
}

VideoFilterType OpenGLDeviceFilter::getOpenGLFilterBaseType() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _openglFilterType;
}

NS_REDRENDER_END
