#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "metal_video_renderer.h"
#include "RedBase.h"
#import "metal_filter_device.h"

NS_REDRENDER_BEGIN

MetalVideoRenderer::MetalVideoRenderer(const int &sessionID)
    : VideoRenderer(sessionID) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

MetalVideoRenderer::~MetalVideoRenderer() {
  AV_LOGI_ID(LOG_TAG, _sessionID, "[%s:%d] start .\n", __FUNCTION__, __LINE__);
  detachAllFilter();
  mMetalVideoCmdQueue = nil;
  if (mMetalView != nil) {
    [mMetalView setRenderer:nil];
  }
  mMetalView = nil;
  if (mCachedPixelBuffer) {
    CVBufferRelease(mCachedPixelBuffer);
    mCachedPixelBuffer = nullptr;
  }
  AV_LOGI_ID(LOG_TAG, _sessionID, "[%s:%d] end .\n", __FUNCTION__, __LINE__);
}

VRError MetalVideoRenderer::initWithFrame(CGRect cgrect) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  mMetalView = [[RedRenderMetalView alloc] initWithFrame:cgrect
                                               sessionID:_sessionID
                                                renderer:this];
  if (nullptr == mMetalView) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] nullptr == [RedRenderMetalView alloc] error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInit;
  }
  mMetalVideoCmdQueue = mMetalView.mMetalVideoCmdQueue;
  if (nullptr == mMetalVideoCmdQueue) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] nullptr == mMetalVideoCmdQueue error .\n", __FUNCTION__,
               __LINE__);
  }

  CGSize drawableSize = mMetalView.mDrawableSize;
  _layerWidth = drawableSize.width;
  _layerHeight = drawableSize.height;

  return VRError::VRErrorNone;
}

UIView *MetalVideoRenderer::getRedRenderView() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return mMetalView;
}

VRError MetalVideoRenderer::attachFilter(VideoFilterType videoFilterType,
                                         VideoFrameMetaData *inputFrameMetaData,
                                         int priority) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  switch (videoFilterType) {
  case VideoMetalDeviceFilterType: {
    if (nullptr == _metalFilterDevice) {
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

  return VRError::VRErrorNone;
}

VRError MetalVideoRenderer::detachFilter(VideoFilterType videoFilterType) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);

  return VRError::VRErrorNone;
}

VRError MetalVideoRenderer::detachAllFilter() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);

  while (!_textureBufferQueue.empty()) {
    _textureBufferQueue.pop();
  }
  for (int i = 0; i < 3; i++) {
    _srcPlaneTextures[i] = nullptr;
  }
  return VRError::VRErrorNone;
}

// input frames for post-processing, and the result is stored in the
// queue:_textureBufferQueue
VRError MetalVideoRenderer::onInputFrame(VideoFrameMetaData *redRenderBuffer) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
  if (mMetalView && [mMetalView isDisplay]) {
    VRError ret = _setInputFrame(redRenderBuffer);
    if (ret != VRError::VRErrorNone) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _setInputImage error.\n",
                 __FUNCTION__, __LINE__);
      return ret;
    }
  }

  return VRError::VRErrorNone;
}

// take frames from the queue:_textureBufferQueue and render them on the screen
VRError MetalVideoRenderer::onRender() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::unique_lock<std::mutex> lck(_rendererMutex);
  if (mMetalView && [mMetalView isDisplay]) {
    _on_screen_render();
  } else {
    _clearTextureBufferQueue();
  }
  return VRError::VRErrorNone;
}

VRError MetalVideoRenderer::onRenderCachedFrame() {
  std::unique_lock<std::mutex> lck(_rendererMutex);
  @autoreleasepool {
    if (mMetalView && [mMetalView isDisplay] && mCachedPixelBuffer) {
      if (mCachedPixelBuffer) {
        CVBufferRetain(mCachedPixelBuffer);
      }
      _srcPlaneTextures[0] = nullptr;
      _srcPlaneTextures[1] = nullptr;
      if (mMetalVideoCmdQueue.textureCache) {
        CVMetalTextureCacheFlush(mMetalVideoCmdQueue.textureCache, 0);
      }

      _srcPlaneTextures[0] = [[MetalVideoTexture alloc]
          initWithPixelBuffer:mMetalVideoCmdQueue.textureCache
                  pixelBuffer:mCachedPixelBuffer
                   planeIndex:0
                    sessionID:_sessionID
                  pixelFormat:MTLPixelFormatR8Unorm];
      _srcPlaneTextures[1] = [[MetalVideoTexture alloc]
          initWithPixelBuffer:mMetalVideoCmdQueue.textureCache
                  pixelBuffer:mCachedPixelBuffer
                   planeIndex:1
                    sessionID:_sessionID
                  pixelFormat:MTLPixelFormatRG8Unorm];

      if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1]) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                   __LINE__);
        _srcPlaneTextures[0] = nullptr;
        _srcPlaneTextures[1] = nullptr;
        _srcPlaneTextures[2] = nullptr;
        if (mCachedPixelBuffer) {
          CVBufferRelease(mCachedPixelBuffer);
        }
        return VRError::VRErrorInputFrame;
      }
      if (mCachedPixelBuffer) {
        CVBufferRelease(mCachedPixelBuffer);
      }

      if (_metalFilterDevice) {
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
  }

  return VRErrorNone;
}

void MetalVideoRenderer::close() {
  if (mMetalView) {
    [mMetalView setRenderer:nil];
  }
}

VRError MetalVideoRenderer::setGravity(
    RedRender::GravityResizeAspectRatio rendererGravity) {
  mRendererGravity = rendererGravity;
  return VRErrorNone;
}

VRError
MetalVideoRenderer::_setInputFrame(VideoFrameMetaData *inputFrameMetaData) {
  if (nullptr == inputFrameMetaData) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] inputFrameMetaData==nullptr error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorInputFrame;
  }
  CGSize drawableSize = mMetalView.mDrawableSize;
  _layerWidth = drawableSize.width;
  _layerHeight = drawableSize.height;
  inputFrameMetaData->aspectRatio = mRendererGravity;

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
  case VRPixelFormatYUV420p: {
    if (nil == inputFrameMetaData->pitches[0] ||
        nil == inputFrameMetaData->pitches[1] ||
        nil == inputFrameMetaData->pitches[2]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nil error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }

    @autoreleasepool {
      if (nil == _srcPlaneTextures[0] || nil == _srcPlaneTextures[1] ||
          nil == _srcPlaneTextures[2] ||
          _onScreenMetaData.frameWidth != inputFrameMetaData->frameWidth ||
          _onScreenMetaData.frameHeight != inputFrameMetaData->frameHeight ||
          _onScreenMetaData.linesize[0] != inputFrameMetaData->linesize[0] ||
          _onScreenMetaData.linesize[1] != inputFrameMetaData->linesize[1] ||
          _onScreenMetaData.linesize[2] != inputFrameMetaData->linesize[2]) {
        _updateInputFrameMetaData(inputFrameMetaData);
        _srcPlaneTextures[0] = nullptr;
        _srcPlaneTextures[1] = nullptr;
        _srcPlaneTextures[2] = nullptr;
      }

      _srcPlaneTextures[0] =
          [[MetalVideoTexture alloc] init:mMetalVideoCmdQueue.device
                                sessionID:_sessionID
                                    width:inputFrameMetaData->linesize[0]
                                   height:inputFrameMetaData->frameHeight
                              pixelFormat:MTLPixelFormatR8Unorm];
      _srcPlaneTextures[1] =
          [[MetalVideoTexture alloc] init:mMetalVideoCmdQueue.device
                                sessionID:_sessionID
                                    width:inputFrameMetaData->linesize[1]
                                   height:inputFrameMetaData->frameHeight / 2
                              pixelFormat:MTLPixelFormatR8Unorm];
      _srcPlaneTextures[2] =
          [[MetalVideoTexture alloc] init:mMetalVideoCmdQueue.device
                                sessionID:_sessionID
                                    width:inputFrameMetaData->linesize[2]
                                   height:inputFrameMetaData->frameHeight / 2
                              pixelFormat:MTLPixelFormatR8Unorm];

      if (nil == _srcPlaneTextures[0] || nil == _srcPlaneTextures[1] ||
          nil == _srcPlaneTextures[2]) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _srcFramebuffer==nil error.\n",
                   __FUNCTION__, __LINE__);
        _srcPlaneTextures[0] = nullptr;
        _srcPlaneTextures[1] = nullptr;
        _srcPlaneTextures[2] = nullptr;
        return VRError::VRErrorInputFrame;
      }

      MTLRegion region[3] = {
          {{0, 0, 0},
           {static_cast<NSUInteger>(inputFrameMetaData->linesize[0]),
            static_cast<NSUInteger>(inputFrameMetaData->frameHeight), 1}},
          {{0, 0, 0},
           {static_cast<NSUInteger>(inputFrameMetaData->linesize[1]),
            static_cast<NSUInteger>(inputFrameMetaData->frameHeight) / 2, 1}},
          {{0, 0, 0},
           {static_cast<NSUInteger>(inputFrameMetaData->linesize[2]),
            static_cast<NSUInteger>(inputFrameMetaData->frameHeight) / 2, 1}},
      };
      [_srcPlaneTextures[0].texture
          replaceRegion:region[0]
            mipmapLevel:0
              withBytes:inputFrameMetaData->pitches[0]
            bytesPerRow:inputFrameMetaData->linesize[0]];
      [_srcPlaneTextures[1].texture
          replaceRegion:region[1]
            mipmapLevel:0
              withBytes:inputFrameMetaData->pitches[1]
            bytesPerRow:inputFrameMetaData->linesize[1]];
      [_srcPlaneTextures[2].texture
          replaceRegion:region[2]
            mipmapLevel:0
              withBytes:inputFrameMetaData->pitches[2]
            bytesPerRow:inputFrameMetaData->linesize[2]];
    }
  } break;
  case VRPixelFormatYUV420sp: {
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

      _srcPlaneTextures[0] =
          [[MetalVideoTexture alloc] init:mMetalVideoCmdQueue.device
                                sessionID:_sessionID
                                    width:inputFrameMetaData->linesize[0]
                                   height:inputFrameMetaData->frameHeight
                              pixelFormat:MTLPixelFormatR8Unorm];
      _srcPlaneTextures[1] =
          [[MetalVideoTexture alloc] init:mMetalVideoCmdQueue.device
                                sessionID:_sessionID
                                    width:inputFrameMetaData->linesize[1] / 2
                                   height:inputFrameMetaData->frameHeight / 2
                              pixelFormat:MTLPixelFormatRG8Unorm];
    }

    if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1]) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                 __LINE__);
      _srcPlaneTextures[0] = nullptr;
      _srcPlaneTextures[1] = nullptr;
      _srcPlaneTextures[2] = nullptr;
      return VRError::VRErrorInputFrame;
    }

    MTLRegion region[2] = {
        {{0, 0, 0},
         {static_cast<NSUInteger>(inputFrameMetaData->linesize[0]),
          static_cast<NSUInteger>(inputFrameMetaData->frameHeight), 1}},
        {{0, 0, 0},
         {static_cast<NSUInteger>(inputFrameMetaData->linesize[1] / 2),
          static_cast<NSUInteger>(inputFrameMetaData->frameHeight) / 2, 1}},
    };
    [_srcPlaneTextures[0].texture
        replaceRegion:region[0]
          mipmapLevel:0
            withBytes:inputFrameMetaData->pitches[0]
          bytesPerRow:inputFrameMetaData->linesize[0]];
    [_srcPlaneTextures[1].texture
        replaceRegion:region[1]
          mipmapLevel:0
            withBytes:inputFrameMetaData->pitches[1]
          bytesPerRow:(inputFrameMetaData->linesize[1])];
  } break;
  case VRPixelFormatYUV420sp_vtb: {
    if (nullptr == inputFrameMetaData->pixel_buffer) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] inputFrameMetaData->pitches==nullptr error.\n",
                 __FUNCTION__, __LINE__);
      return VRError::VRErrorInputFrame;
    }
    @autoreleasepool {
      if (inputFrameMetaData->pixel_buffer) {
        CVBufferRetain(inputFrameMetaData->pixel_buffer);
      }

      VRAVColorSpace colorspace =
          [mMetalView getColorSpace:inputFrameMetaData->pixel_buffer];
      if (colorspace != VR_AVCOL_SPC_NB) {
        inputFrameMetaData->color_space = colorspace;
      }

      _srcPlaneTextures[0] = nullptr;
      _srcPlaneTextures[1] = nullptr;
      if (mMetalVideoCmdQueue.textureCache) {
        CVMetalTextureCacheFlush(mMetalVideoCmdQueue.textureCache, 0);
      }

      _srcPlaneTextures[0] = [[MetalVideoTexture alloc]
          initWithPixelBuffer:mMetalVideoCmdQueue.textureCache
                  pixelBuffer:inputFrameMetaData->pixel_buffer
                   planeIndex:0
                    sessionID:_sessionID
                  pixelFormat:MTLPixelFormatR8Unorm];
      _srcPlaneTextures[1] = [[MetalVideoTexture alloc]
          initWithPixelBuffer:mMetalVideoCmdQueue.textureCache
                  pixelBuffer:inputFrameMetaData->pixel_buffer
                   planeIndex:1
                    sessionID:_sessionID
                  pixelFormat:MTLPixelFormatRG8Unorm];
      if (nullptr == _srcPlaneTextures[0] || nullptr == _srcPlaneTextures[1]) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] _srcFramebuffer==nullptr error.\n", __FUNCTION__,
                   __LINE__);
        _srcPlaneTextures[0] = nullptr;
        _srcPlaneTextures[1] = nullptr;
        _srcPlaneTextures[2] = nullptr;
        if (inputFrameMetaData->pixel_buffer) {
          CVBufferRelease(inputFrameMetaData->pixel_buffer);
        }
        return VRError::VRErrorInputFrame;
      }

      if (mCachedPixelBuffer) {
        CVBufferRelease(mCachedPixelBuffer);
      }
      mCachedPixelBuffer = inputFrameMetaData->pixel_buffer;
      if (mCachedPixelBuffer) {
        CVBufferRetain(mCachedPixelBuffer);
      }
      if (inputFrameMetaData->pixel_buffer) {
        CVBufferRelease(inputFrameMetaData->pixel_buffer);
      }
    }
  } break;
  default:
    break;
  }

  _updateInputFrameMetaData(inputFrameMetaData);

  if (nil == _metalFilterDevice ||
      _metalFilterDevice.pixelFormat != inputFrameMetaData->pixel_format) {
    _metalFilterDevice = nil;
    VRError ret = _createOnScreenRender(&_onScreenMetaData);
    if (ret != VRErrorNone) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _createOnScreenRender error.\n",
                 __FUNCTION__, __LINE__);
      return ret;
    }
  }

  if (_metalFilterDevice) {
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

VRError MetalVideoRenderer::_on_screen_render() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (nil == _metalFilterDevice) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _metalFilterDevice==nil error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorOnRender;
  }
  if (nullptr == _inputFrameMetaData) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] _inputFrameMetaData==nullptr error .\n", __FUNCTION__,
               __LINE__);
    return VRError::VRErrorOnRender;
  }
  MetalVideoTexture *texture[3] = {nullptr};

  switch (_inputFrameMetaData->pixel_format) {
  case VRPixelFormatRGB565:
  case VRPixelFormatRGB888:
  case VRPixelFormatRGBX8888:
    if (!_textureBufferQueue.empty()) {
      texture[0] = _textureBufferQueue.front();
      _textureBufferQueue.pop();
      [_metalFilterDevice setInputTexture:texture[0]
                             rotationMode:_inputFrameMetaData->rotationMode
                        inputTextureIndex:0];
      texture[0] = nullptr; // release input framebuffer
    }
    break;
  case VRPixelFormatYUV420p:
    for (int i = 0; i < 3; i++) {
      if (!_textureBufferQueue.empty()) {
        texture[i] = _textureBufferQueue.front();
        _textureBufferQueue.pop();
        [_metalFilterDevice setInputTexture:texture[i]
                               rotationMode:_inputFrameMetaData->rotationMode
                          inputTextureIndex:i];
        texture[i] = nullptr; // release input framebuffer
      }
    }
    break;
  case VRPixelFormatYUV420sp:
  case VRPixelFormatYUV420sp_vtb:
    for (int i = 0; i < 2; i++) {
      if (!_textureBufferQueue.empty()) {
        texture[i] = _textureBufferQueue.front();
        _textureBufferQueue.pop();
        [_metalFilterDevice setInputTexture:texture[i]
                               rotationMode:_inputFrameMetaData->rotationMode
                          inputTextureIndex:i];
        texture[i] = nullptr; // release input framebuffer
      }
    }
    break;
  default:
    break;
  }
  @autoreleasepool {
    if ([_metalFilterDevice isPrepared]) {
      [_metalFilterDevice setInputFrameMetaData:_inputFrameMetaData];
      BOOL ret = [_metalFilterDevice updateParam:mMetalView];
      if (!ret) {
        AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] updateParam error .\n",
                   __FUNCTION__, __LINE__);
        [_metalFilterDevice clearInputTexture];
        _srcPlaneTextures[0] = nullptr;
        _srcPlaneTextures[1] = nullptr;
        _srcPlaneTextures[2] = nullptr;
        return VRError::VRErrorOnRender;
      }

      [_metalFilterDevice releaseSharedcmdBuffer];
      [_metalFilterDevice newSharedcmdBuffer:mMetalVideoCmdQueue];

      [_metalFilterDevice onRender:mMetalView];
      [_metalFilterDevice releaseSharedcmdBuffer];

      [_metalFilterDevice clearInputTexture];
      _srcPlaneTextures[0] = nullptr;
      _srcPlaneTextures[1] = nullptr;
      _srcPlaneTextures[2] = nullptr;

      // fps
      uint64_t current = CurrentTimeMs();
      uint64_t delta =
          (current > _lastFrameTime) ? current - _lastFrameTime : 0;
      if (delta <= 0) {
        _lastFrameTime = current;
      } else if (delta >= 1000) {
        _fps = ((double)_frameCount) * 1000 / delta;
        _lastFrameTime = current;
        _frameCount = 0;
      } else {
        _frameCount++;
      }
      AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] metal render fps : %lf .\n",
                 __FUNCTION__, __LINE__, _fps);
    }
  }

  return VRError::VRErrorNone;
}

VRError MetalVideoRenderer::_createOnScreenRender(
    VideoFrameMetaData *inputFrameMetaData) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (!mMetalVideoCmdQueue || !inputFrameMetaData) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] mMetalVideoCmdQueue==nullptr || "
               "inputFrameMetaData==nullptr error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInitFilter;
  }
  _metalFilterDevice = [[MetalFilterDevice alloc] init:mMetalVideoCmdQueue
                                    inputFrameMetaData:inputFrameMetaData
                                             sessionID:_sessionID];
  if (nil == _metalFilterDevice) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] _metalFilterDevice==nil error .\n",
               __FUNCTION__, __LINE__);
    return VRError::VRErrorInitFilter;
  }
  return VRError::VRErrorNone;
}

void MetalVideoRenderer::_clearTextureBufferQueue() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  while (!_textureBufferQueue.empty()) {
    _textureBufferQueue.pop();
  }

  for (int i = 0; i < 3; i++) {
    _srcPlaneTextures[i] = nullptr;
  }
}

void MetalVideoRenderer::_updateInputFrameMetaData(
    VideoFrameMetaData *inputFrameMetaData) {
  _onScreenMetaData = {
      .pitches[0] = inputFrameMetaData->pitches[0],
      .pitches[1] = inputFrameMetaData->pitches[1],
      .pitches[2] = inputFrameMetaData->pitches[2],
      .linesize[0] = inputFrameMetaData->linesize[0],
      .linesize[1] = inputFrameMetaData->linesize[1],
      .linesize[2] = inputFrameMetaData->linesize[2],
      .stride = inputFrameMetaData->stride,
      .layerWidth = inputFrameMetaData->layerWidth,
      .layerHeight = inputFrameMetaData->layerHeight,
      .frameWidth = inputFrameMetaData->frameWidth,
      .frameHeight = inputFrameMetaData->frameHeight,
      .pixel_format = inputFrameMetaData->pixel_format,
      .color_primaries = inputFrameMetaData->color_primaries,
      .color_trc = inputFrameMetaData->color_trc,
      .color_space = inputFrameMetaData->color_space,
      .color_range = inputFrameMetaData->color_range,
      .aspectRatio = inputFrameMetaData->aspectRatio,
      .rotationMode = inputFrameMetaData->rotationMode,
      .sar = inputFrameMetaData->sar,
  };
}

NS_REDRENDER_END
#endif
