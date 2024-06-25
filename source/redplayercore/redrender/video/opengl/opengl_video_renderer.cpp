/*
 * opengl_video_renderer.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./opengl_video_renderer.h"

#include "RedBase.h"
#include "common/context.h"
#include "common/framebuffer.h"
#include "filter/opengl_device_filter.h"
#include "input/opengl_video_input.h"
#include "output/opengl_video_output.h"
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
#include "android/egl_context.h"

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
#include "harmony/egl_context.h"

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "ios/eagl_context.h"
#endif

NS_REDRENDER_BEGIN

OpenGLVideoRenderer::OpenGLVideoRenderer(const int &sessionID)
    : VideoRenderer(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

OpenGLVideoRenderer::~OpenGLVideoRenderer() {
  releaseContext();
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

VRError OpenGLVideoRenderer::releaseContext() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  mInstance.reset();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  mRedRenderGLView = nullptr;
  mRedRenderGLTexture[0] = nullptr;
  mRedRenderGLTexture[1] = nullptr;
  if (mCachedPixelBuffer) {
    CVBufferRelease(mCachedPixelBuffer);
    mCachedPixelBuffer = nullptr;
  }
#endif

  return VRError::VRErrorNone;
}

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
VRError OpenGLVideoRenderer::init(ANativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (mInstance) {
    mInstance.reset();
  }
  mInstance = Context::creatInstance(_sessionID);
  if (nullptr == mInstance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], Context::creatInstance error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInit;
  }
  return VRError::VRErrorNone;
}

VRError
OpenGLVideoRenderer::setSurface(ANativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
  if (nullptr == mInstance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], mInstance==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (nullptr == nativeWindow) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], nativeWindow==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorWindow;
  }
  if (nullptr == mInstance->getEglContext()) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], _eglContext==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (!mInstance->getEglContext()->init()) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], context init error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (!mInstance->getEglContext()->createEGLSurfaceWithWindow(nativeWindow)) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d], createEGLSurfaceWithWindow error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorContext;
  }

  mInstance->opengl_print_string("Version", GL_VERSION);
  mInstance->opengl_print_string("Vendor", GL_VENDOR);
  mInstance->opengl_print_string("Renderer", GL_RENDERER);
  mInstance->opengl_print_string("Extensions", GL_EXTENSIONS);

  _layerWidth = mInstance->getEglContext()->getSurfaceWidth();
  _layerHeight = mInstance->getEglContext()->getSurfaceHeight();

  return VRError::VRErrorNone;
}

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY
VRError
OpenGLVideoRenderer::init(OHNativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (mInstance) {
    mInstance.reset();
  }
  mInstance = Context::creatInstance(_sessionID);
  if (nullptr == mInstance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], Context::creatInstance error .\n",
               __FUNCTION__, __LINE__);
  }
  return VRError::VRErrorNone;
}

VRError
OpenGLVideoRenderer::setSurface(OHNativeWindow *nativeWindow /* = nullptr */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
  if (nullptr == mInstance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], mInstance==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (0 == nativeWindow) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], nativeWindow==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorWindow;
  }
  if (nullptr == mInstance->getEglContext()) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], _eglContext==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (!mInstance->getEglContext()->init()) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], context init error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (!mInstance->getEglContext()->createEGLSurfaceWithWindow(nativeWindow)) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d], createEGLSurfaceWithWindow error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorContext;
  }

  mInstance->opengl_print_string("Version", GL_VERSION);
  mInstance->opengl_print_string("Vendor", GL_VENDOR);
  mInstance->opengl_print_string("Renderer", GL_RENDERER);
  mInstance->opengl_print_string("Extensions", GL_EXTENSIONS);

  _layerWidth = mInstance->getEglContext()->getSurfaceWidth();
  _layerHeight = mInstance->getEglContext()->getSurfaceHeight();

  return VRError::VRErrorNone;
}

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
VRError OpenGLVideoRenderer::init() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (mInstance) {
    mInstance.reset();
  }
  mInstance = Context::creatInstance(_sessionID);
  if (nullptr == mInstance) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], Context::creatInstance error .\n",
               __FUNCTION__, __LINE__);
  }
  return VRError::VRErrorNone;
}

VRError
OpenGLVideoRenderer::initWithFrame(CGRect cgrect /* = {{0,0}, {0,0}} */) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (nullptr == mInstance->getEaglContext()) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], _eglContext==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  if (!mInstance->getEaglContext()->init(cgrect, this)) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], context init error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorContext;
  }
  mInstance->opengl_print_string("Version", GL_VERSION);
  mInstance->opengl_print_string("Vendor", GL_VENDOR);
  mInstance->opengl_print_string("Renderer", GL_RENDERER);
  mInstance->opengl_print_string("Extensions", GL_EXTENSIONS);

  _layerWidth = mInstance->getEaglContext()->getFrameWidth();
  _layerHeight = mInstance->getEaglContext()->getFrameHeight();

  return VRError::VRErrorNone;
}

UIView *OpenGLVideoRenderer::getRedRenderView() {
  mRedRenderGLView = mInstance->getEaglContext()->getRedRenderGLView();
  if (nullptr == mRedRenderGLView) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d], context init error .\n",
               __FUNCTION__, __LINE__);
    return nullptr;
  }
  return mRedRenderGLView;
}

#endif

VRError OpenGLVideoRenderer::setGravity(
    RedRender::GravityResizeAspectRatio rendererGravity) {
  mRendererGravity = rendererGravity;

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  _layerWidth = mInstance->getEaglContext()->getFrameWidth();
  _layerHeight = mInstance->getEaglContext()->getFrameHeight();
  if (_inputFrameMetaData != nullptr) {
    _inputFrameMetaData->layerWidth = _layerWidth;
    _inputFrameMetaData->layerHeight = _layerHeight;
  }
#endif

  return VRErrorNone;
}

VRError
OpenGLVideoRenderer::attachFilter(VideoFilterType videoFilterType,
                                  VideoFrameMetaData *inputFrameMetaData,
                                  int priority) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  mInstance->getEaglContext()->useAsCurrent();
#endif
  std::shared_ptr<OpenGLFilterBase> filter = nullptr;
  switch (videoFilterType) {
  case VideoOpenGLDeviceFilterType: {
    if (nullptr == _openglFilterDevice) {
      _updateInputFrameMetaData(inputFrameMetaData);
      VRError ret = _createOnScreenRender(&_onScreenMetaData);
      if (ret != VRErrorNone) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] _createOnScreenRender error.\n", __FUNCTION__,
                   __LINE__);
        return ret;
      }
    }
  } break;
  default:
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] does not support this filter videoFilterType:%d.\n",
               __FUNCTION__, __LINE__, (int)videoFilterType);
    break;
  }
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  mInstance->getEaglContext()->useAsPrev();
#endif
  return VRError::VRErrorNone;
}

// input frames for post-processing, and the result is stored in the
// queue:_textureBufferQueue
VRError OpenGLVideoRenderer::onInputFrame(VideoFrameMetaData *redRenderBuffer) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  if (mRedRenderGLView && mInstance->getEaglContext()->isDisplay()) {
    mInstance->getEaglContext()->useAsCurrent();
    VRError ret = _setInputFrame(redRenderBuffer);
    if (ret != VRError::VRErrorNone) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _setInputImage error.\n",
                 __FUNCTION__, __LINE__);
      return ret;
    }
    mInstance->getEaglContext()->useAsPrev();
  }

#elif (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                    \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  VRError ret = _setInputFrame(redRenderBuffer);
  if (ret != VRError::VRErrorNone) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _setInputImage error.\n",
               __FUNCTION__, __LINE__);
    return ret;
  }

#endif

  return VRError::VRErrorNone;
}

// take frames from the queue:_textureBufferQueue and render them on the screen
VRError OpenGLVideoRenderer::onRender() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  if (mRedRenderGLView && mInstance->getEaglContext()->isDisplay()) {
    mInstance->getEaglContext()->useAsCurrent();
    _on_screen_render();
    mInstance->getEaglContext()->useAsPrev();
  }
#elif (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                    \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  _on_screen_render();
#endif
  return VRError::VRErrorNone;
}

VRError OpenGLVideoRenderer::onRenderCachedFrame() {
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  std::unique_lock<std::mutex> lck(_rendererMutex);
  if (mRedRenderGLView && mInstance->getEaglContext()->isDisplay() &&
      mCachedPixelBuffer) {
    if (mCachedPixelBuffer) {
      CVBufferRetain(mCachedPixelBuffer);
    }
    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1]) {
      if (nullptr != _srcPlaneTextures[0]) {
        _srcPlaneTextures[0].reset();
      }
      if (nullptr != _srcPlaneTextures[1]) {
        _srcPlaneTextures[1].reset();
      }
      _srcPlaneTextures[0] = Framebuffer::create(_sessionID);
      _srcPlaneTextures[1] = Framebuffer::create(_sessionID);
    }

    VRAVColorSpace colorspace = mInstance->getEaglContext()->getColorSpace(
        _inputFrameMetaData->pixel_buffer);
    if (colorspace != VR_AVCOL_SPC_NB) {
      _inputFrameMetaData->color_space = colorspace;
    }

    glActiveTexture(GL_TEXTURE0);
    mRedRenderGLTexture[0] =
        mInstance->getEaglContext()->getGLTextureWithPixelBuffer(
            mRedRenderGLView.textureCache, mCachedPixelBuffer, 0, _sessionID,
            GL_TEXTURE_2D, GL_RED_EXT, GL_UNSIGNED_BYTE);
    if (mRedRenderGLTexture[0] == nullptr) {
      if (mCachedPixelBuffer) {
        CVBufferRelease(mCachedPixelBuffer);
      }
      mRedRenderGLTexture[0] = nullptr;
      mRedRenderGLTexture[1] = nullptr;
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] mRedRenderGLTexture[0]==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    _srcPlaneTextures[0]->setTexture(mRedRenderGLTexture[0].texture);
    _srcPlaneTextures[0]->setWidth(mRedRenderGLTexture[0].width);
    _srcPlaneTextures[0]->setHeight(mRedRenderGLTexture[0].height);
    glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[0]->getTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    mRedRenderGLTexture[1] =
        mInstance->getEaglContext()->getGLTextureWithPixelBuffer(
            mRedRenderGLView.textureCache, mCachedPixelBuffer, 1, _sessionID,
            GL_TEXTURE_2D, GL_RG_EXT, GL_UNSIGNED_BYTE);
    if (mRedRenderGLTexture[1] == nullptr) {
      if (mCachedPixelBuffer) {
        CVBufferRelease(mCachedPixelBuffer);
      }
      mRedRenderGLTexture[0] = nullptr;
      mRedRenderGLTexture[1] = nullptr;
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] mRedRenderGLTexture[1]==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    _srcPlaneTextures[1]->setTexture(mRedRenderGLTexture[1].texture);
    _srcPlaneTextures[1]->setWidth(mRedRenderGLTexture[1].width);
    _srcPlaneTextures[1]->setHeight(mRedRenderGLTexture[1].height);
    glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[1]->getTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (mCachedPixelBuffer) {
      CVBufferRelease(mCachedPixelBuffer);
    }
    if (_openglFilterDevice) {
      if (_srcPlaneTextures[0]) {
        _textureBufferQueue.push(_srcPlaneTextures[0]);
        _inputFrameMetaData = &_onScreenMetaData;
      }
      if (_srcPlaneTextures[1]) {
        _textureBufferQueue.push(_srcPlaneTextures[1]);
      }
      if (_srcPlaneTextures[2]) {
        _textureBufferQueue.push(_srcPlaneTextures[2]);
      }
    }
    _on_screen_render();
  }
#endif
  return VRErrorNone;
}

VRError
OpenGLVideoRenderer::_setInputFrame(VideoFrameMetaData *inputFrameMetaData) {
  if (inputFrameMetaData == nullptr) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] inputFrameMetaData == nullptr error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorInputFrame;
  }
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
  EGLBoolean res = mInstance->getEglContext()->makeCurrent();
  if (!res) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] makeCurrent error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInputFrame;
  }
#endif
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  inputFrameMetaData->aspectRatio = mRendererGravity;
#endif
  inputFrameMetaData->layerWidth = _layerWidth;
  inputFrameMetaData->layerHeight = _layerHeight;
  AV_LOGV_ID(
      LOG_TAG, _sessionID,
      "[%s:%d] .\n"
      "pitches[0] : %p, pitches[1] : %p, pitches[2] : %p .\n"
      "stride : %d, linesize[0] : %d, linesize[1] : %d, linesize[2] : %d .\n"
      "layerWidth : %d, layerHeight : %d, frameWidth : %d, frameHeight : %d .\n"
      "pixel_format : %d, color_primaries : %d, color_trc : %d,  color_space : "
      "%d .\n"
      "color_range : %d, aspectRatio : %d, sar : %d/%d .\n",
      __FUNCTION__, __LINE__, inputFrameMetaData->pitches[0],
      inputFrameMetaData->pitches[1], inputFrameMetaData->pitches[2],
      inputFrameMetaData->stride, inputFrameMetaData->linesize[0],
      inputFrameMetaData->linesize[1], inputFrameMetaData->linesize[2],
      inputFrameMetaData->layerWidth, inputFrameMetaData->layerHeight,
      inputFrameMetaData->frameWidth, inputFrameMetaData->frameHeight,
      inputFrameMetaData->pixel_format, inputFrameMetaData->color_primaries,
      inputFrameMetaData->color_trc, inputFrameMetaData->color_space,
      inputFrameMetaData->color_range, inputFrameMetaData->aspectRatio,
      inputFrameMetaData->sar.num, inputFrameMetaData->sar.den);

  switch (inputFrameMetaData->pixel_format) {
  case VRPixelFormatRGB565:
    break;
  case VRPixelFormatRGB888:
    break;
  case VRPixelFormatRGBX8888:
    if (nullptr == inputFrameMetaData->pitches[0]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    if (nullptr == _srcPlaneTextures[0] ||
        _onScreenMetaData.frameWidth != inputFrameMetaData->frameWidth ||
        _onScreenMetaData.frameHeight != inputFrameMetaData->frameHeight ||
        _onScreenMetaData.linesize[0] != inputFrameMetaData->linesize[0]) {
      _updateInputFrameMetaData(inputFrameMetaData);
      if (nullptr != _srcPlaneTextures[0]) {
        _srcPlaneTextures[0].reset();
      }
      _srcPlaneTextures[0] = Framebuffer::create(
          inputFrameMetaData->linesize[0], inputFrameMetaData->frameHeight,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
    }
    if (nullptr == _srcPlaneTextures[0]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                 __LINE__);
      return VRError::VRErrorInputFrame;
    }
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[0]->getTexture()));
    CHECK_OPENGL(
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inputFrameMetaData->linesize[0],
                     inputFrameMetaData->frameHeight, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, inputFrameMetaData->pitches[0]));
    CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
    break;
  case VRPixelFormatYUV420p:
    if (nullptr == inputFrameMetaData->pitches[0] ||
        nullptr == inputFrameMetaData->pitches[1] ||
        nullptr == inputFrameMetaData->pitches[2]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }

    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1] ||
        nullptr == _srcPlaneTextures[2] ||
        _onScreenMetaData.frameWidth != inputFrameMetaData->frameWidth ||
        _onScreenMetaData.frameHeight != inputFrameMetaData->frameHeight ||
        _onScreenMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
        _onScreenMetaData.linesize[1] != inputFrameMetaData->linesize[1] ||
        _onScreenMetaData.linesize[2] != inputFrameMetaData->linesize[2]) {
      _updateInputFrameMetaData(inputFrameMetaData);
      if (nullptr != _srcPlaneTextures[0]) {
        _srcPlaneTextures[0].reset();
      }
      if (nullptr != _srcPlaneTextures[1]) {
        _srcPlaneTextures[1].reset();
      }
      if (nullptr != _srcPlaneTextures[2]) {
        _srcPlaneTextures[2].reset();
      }
      _srcPlaneTextures[0] = Framebuffer::create(
          inputFrameMetaData->linesize[0], inputFrameMetaData->frameHeight,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
      _srcPlaneTextures[1] = Framebuffer::create(
          inputFrameMetaData->linesize[1], inputFrameMetaData->frameHeight / 2,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
      _srcPlaneTextures[2] = Framebuffer::create(
          inputFrameMetaData->linesize[2], inputFrameMetaData->frameHeight / 2,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
    }

    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1] ||
        nullptr == _srcPlaneTextures[2]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                 __LINE__);
      return VRError::VRErrorInputFrame;
    }
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[0]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[0],
        inputFrameMetaData->frameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[0]));
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[1]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[1],
        inputFrameMetaData->frameHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[1]));
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[2]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[2],
        inputFrameMetaData->frameHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[2]));
    break;
  case VRPixelFormatYUV420sp:
    if (nullptr == inputFrameMetaData->pitches[0] ||
        nullptr == inputFrameMetaData->pitches[1]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1] ||
        _onScreenMetaData.frameWidth != inputFrameMetaData->frameWidth ||
        _onScreenMetaData.frameHeight != inputFrameMetaData->frameHeight ||
        _onScreenMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
        _onScreenMetaData.linesize[1] != inputFrameMetaData->linesize[1]) {
      _updateInputFrameMetaData(inputFrameMetaData);
      if (nullptr != _srcPlaneTextures[0]) {
        _srcPlaneTextures[0].reset();
      }
      if (nullptr != _srcPlaneTextures[1]) {
        _srcPlaneTextures[1].reset();
      }
      _srcPlaneTextures[0] = Framebuffer::create(
          inputFrameMetaData->linesize[0], inputFrameMetaData->frameHeight,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
      _srcPlaneTextures[1] = Framebuffer::create(
          inputFrameMetaData->linesize[1] / 2,
          inputFrameMetaData->frameHeight / 2, true,
          Framebuffer::defaultTextureAttribures, _sessionID);
    }
    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                 __LINE__);
      return VRError::VRErrorInputFrame;
    }
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[0]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[0],
        inputFrameMetaData->frameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[0]));
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[1]->getTexture()));
    CHECK_OPENGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                              inputFrameMetaData->linesize[1] / 2,
                              inputFrameMetaData->frameHeight / 2, 0,
                              GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                              inputFrameMetaData->pitches[1]));
    CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
    break;
  case VRPixelFormatYUV420sp_vtb: {
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
    if (nullptr == inputFrameMetaData->pixel_buffer) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }

    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1]) {
      if (nullptr != _srcPlaneTextures[0]) {
        _srcPlaneTextures[0].reset();
      }
      if (nullptr != _srcPlaneTextures[1]) {
        _srcPlaneTextures[1].reset();
      }
      _srcPlaneTextures[0] = Framebuffer::create(_sessionID);
      _srcPlaneTextures[1] = Framebuffer::create(_sessionID);
    }
    if (inputFrameMetaData->pixel_buffer) {
      CVBufferRetain(inputFrameMetaData->pixel_buffer);
    }

    VRAVColorSpace colorspace = mInstance->getEaglContext()->getColorSpace(
        inputFrameMetaData->pixel_buffer);
    if (colorspace != VR_AVCOL_SPC_NB) {
      inputFrameMetaData->color_space = colorspace;
    }

    glActiveTexture(GL_TEXTURE0);
    mRedRenderGLTexture[0] =
        mInstance->getEaglContext()->getGLTextureWithPixelBuffer(
            mRedRenderGLView.textureCache, inputFrameMetaData->pixel_buffer, 0,
            _sessionID, GL_TEXTURE_2D, GL_RED_EXT, GL_UNSIGNED_BYTE);
    if (mRedRenderGLTexture[0] == nullptr) {
      if (inputFrameMetaData->pixel_buffer) {
        CVBufferRelease(inputFrameMetaData->pixel_buffer);
      }
      mRedRenderGLTexture[0] = nullptr;
      mRedRenderGLTexture[1] = nullptr;
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] mRedRenderGLTexture[0]==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    _srcPlaneTextures[0]->setTexture(mRedRenderGLTexture[0].texture);
    _srcPlaneTextures[0]->setWidth(mRedRenderGLTexture[0].width);
    _srcPlaneTextures[0]->setHeight(mRedRenderGLTexture[0].height);
    glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[0]->getTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    mRedRenderGLTexture[1] =
        mInstance->getEaglContext()->getGLTextureWithPixelBuffer(
            mRedRenderGLView.textureCache, inputFrameMetaData->pixel_buffer, 1,
            _sessionID, GL_TEXTURE_2D, GL_RG_EXT, GL_UNSIGNED_BYTE);
    if (mRedRenderGLTexture[1] == nullptr) {
      if (inputFrameMetaData->pixel_buffer) {
        CVBufferRelease(inputFrameMetaData->pixel_buffer);
      }
      mRedRenderGLTexture[0] = nullptr;
      mRedRenderGLTexture[1] = nullptr;
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] mRedRenderGLTexture[1]==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    _srcPlaneTextures[1]->setTexture(mRedRenderGLTexture[1].texture);
    _srcPlaneTextures[1]->setWidth(mRedRenderGLTexture[1].width);
    _srcPlaneTextures[1]->setHeight(mRedRenderGLTexture[1].height);
    glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[1]->getTexture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (mCachedPixelBuffer) {
      CVBufferRelease(mCachedPixelBuffer);
      mCachedPixelBuffer = nullptr;
    }
    mCachedPixelBuffer = inputFrameMetaData->pixel_buffer;
    if (mCachedPixelBuffer) {
      CVBufferRetain(mCachedPixelBuffer);
    }
    if (inputFrameMetaData->pixel_buffer) {
      CVBufferRelease(inputFrameMetaData->pixel_buffer);
    }
#endif
  } break;
  case VRPixelFormatYUV420p10le:
    if (nullptr == inputFrameMetaData->pitches[0] ||
        nullptr == inputFrameMetaData->pitches[1] ||
        nullptr == inputFrameMetaData->pitches[2]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }

    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1] ||
        nullptr == _srcPlaneTextures[2] ||
        _onScreenMetaData.frameWidth != inputFrameMetaData->frameWidth ||
        _onScreenMetaData.frameHeight != inputFrameMetaData->frameHeight ||
        _onScreenMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
        _onScreenMetaData.linesize[1] != inputFrameMetaData->linesize[1] ||
        _onScreenMetaData.linesize[2] != inputFrameMetaData->linesize[2]) {
      _updateInputFrameMetaData(inputFrameMetaData);
      if (nullptr != _srcPlaneTextures[0]) {
        _srcPlaneTextures[0].reset();
      }
      if (nullptr != _srcPlaneTextures[1]) {
        _srcPlaneTextures[1].reset();
      }
      if (nullptr != _srcPlaneTextures[2]) {
        _srcPlaneTextures[2].reset();
      }
      _srcPlaneTextures[0] = Framebuffer::create(
          inputFrameMetaData->linesize[0] / 2, inputFrameMetaData->frameHeight,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
      _srcPlaneTextures[1] = Framebuffer::create(
          inputFrameMetaData->linesize[1] / 2, inputFrameMetaData->frameHeight,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
      _srcPlaneTextures[2] = Framebuffer::create(
          inputFrameMetaData->linesize[2] / 2, inputFrameMetaData->frameHeight,
          true, Framebuffer::defaultTextureAttribures, _sessionID);
    }

    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1] ||
        nullptr == _srcPlaneTextures[2]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                 __LINE__);
      return VRError::VRErrorInputFrame;
    }
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[0]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[0] / 2,
        inputFrameMetaData->frameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[0]));
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[1]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[1] / 2,
        inputFrameMetaData->frameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[1]));
    CHECK_OPENGL(
        glBindTexture(GL_TEXTURE_2D, _srcPlaneTextures[2]->getTexture()));
    CHECK_OPENGL(glTexImage2D(
        GL_TEXTURE_2D, 0, GL_LUMINANCE, inputFrameMetaData->linesize[2] / 2,
        inputFrameMetaData->frameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
        inputFrameMetaData->pitches[2]));
    break;
  case VRPixelFormatMediaCodec:
    break;
  case VRPixelFormatHisiMediaCodec:
    break;
  case VRPixelFormatHarmonyVideoDecoder:
    break;
  default:
    break;
  }

  _updateInputFrameMetaData(inputFrameMetaData);

  if (nullptr == _openglFilterDevice ||
      _openglFilterDevice->pixelFormat != inputFrameMetaData->pixel_format) {
    if (_openglFilterDevice != nullptr) {
      _openglFilterDevice.reset();
      _openglFilterDevice = nullptr;
    }
    VRError ret = _createOnScreenRender(&_onScreenMetaData);
    if (ret != VRErrorNone) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _createOnScreenRender error.\n",
                 __FUNCTION__, __LINE__);
      return ret;
    }
  }

  if (_openglFilterDevice) {
    if (_srcPlaneTextures[0]) {
      _textureBufferQueue.push(_srcPlaneTextures[0]);
      _inputFrameMetaData = inputFrameMetaData;
    }
    if (_srcPlaneTextures[1]) {
      _textureBufferQueue.push(_srcPlaneTextures[1]);
    }
    if (_srcPlaneTextures[2]) {
      _textureBufferQueue.push(_srcPlaneTextures[2]);
    }
  }
  return VRError::VRErrorNone;
}

VRError OpenGLVideoRenderer::_on_screen_render() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (nullptr == _openglFilterDevice) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] _openglFilterDevice==nullptr error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorOnRender;
  }
  if (nullptr == _inputFrameMetaData) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] _inputFrameMetaData==nullptr error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorOnRender;
  }
  std::shared_ptr<Framebuffer> texture[3] = {nullptr};
  switch (_inputFrameMetaData->pixel_format) {
  case VRPixelFormatRGB565:
  case VRPixelFormatRGB888:
  case VRPixelFormatRGBX8888:
    if (!_textureBufferQueue.empty()) {
      texture[0] = _textureBufferQueue.front();
      _textureBufferQueue.pop();
      _openglFilterDevice->setInputFramebuffer(
          texture[0], _inputFrameMetaData->rotationMode, 0);
      texture[0].reset(); // release input framebuffer
    }
    break;
  case VRPixelFormatYUV420p:
  case VRPixelFormatYUV420sp:
    for (int i = 0; i < 3; i++) {
      if (!_textureBufferQueue.empty()) {
        texture[i] = _textureBufferQueue.front();
        _textureBufferQueue.pop();
        _openglFilterDevice->setInputFramebuffer(
            texture[i], _inputFrameMetaData->rotationMode, i);
        texture[i].reset(); // release input framebuffer
      }
    }
    break;
  case VRPixelFormatYUV420sp_vtb:
    for (int i = 0; i < 2; i++) {
      if (!_textureBufferQueue.empty()) {
        texture[i] = _textureBufferQueue.front();
        _textureBufferQueue.pop();
        _openglFilterDevice->setInputFramebuffer(
            texture[i], _inputFrameMetaData->rotationMode, i);
        texture[i].reset(); // release input framebuffer
      }
    }
    break;
  case VRPixelFormatYUV420p10le:
    break;
  case VRPixelFormatMediaCodec:
    break;
  case VRPixelFormatHisiMediaCodec:
    break;
  case VRPixelFormatHarmonyVideoDecoder:
    break;
  default:
    break;
  }
  if (_openglFilterDevice->isPrepared()) {
#if (REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID) ||                      \
    (REDRENDER_PLATFORM == REDRENDER_PLATFORM_HARMONY)
    EGLBoolean res = mInstance->getEglContext()->makeCurrent();
    if (!res) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] makeCurrent error .\n",
                 __FUNCTION__, __LINE__);
      return VRErrorContext;
    }
#endif
    _openglFilterDevice->setInputFrameMetaData(_inputFrameMetaData);
    _openglFilterDevice->updateParam();
    _openglFilterDevice->onRender();
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
    mRedRenderGLTexture[0] = nullptr;
    mRedRenderGLTexture[1] = nullptr;
#endif

    // fps
    uint64_t current = CurrentTimeMs();
    uint64_t delta = (current > _lastFrameTime) ? current - _lastFrameTime : 0;
    if (delta <= 0) {
      _lastFrameTime = current;
    } else if (delta >= 1000) {
      _fps = (static_cast<double>(_frameCount)) * 1000 / delta;
      _lastFrameTime = current;
      _frameCount = 0;
    } else {
      _frameCount++;
    }
    AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] opengl render fps : %lf .\n",
               __FUNCTION__, __LINE__, _fps);
  }

  return VRError::VRErrorNone;
}

VRError OpenGLVideoRenderer::_createOnScreenRender(
    VideoFrameMetaData *inputFrameMetaData) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _openglFilterDevice =
      OpenGLDeviceFilter::create(inputFrameMetaData, _sessionID, mInstance);
  if (nullptr == _openglFilterDevice) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] _openglFilterDevice nullptr error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorInitFilter;
  }

  return VRError::VRErrorNone;
}

void OpenGLVideoRenderer::_updateInputFrameMetaData(
    VideoFrameMetaData *inputFrameMetaData) {
  _onScreenMetaData = *inputFrameMetaData;
}

NS_REDRENDER_END
