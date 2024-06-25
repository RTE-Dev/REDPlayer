/*
 * metal_filter_base.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_filter_base.h"
#import <simd/simd.h>

@implementation MetalFilterBase

- (id)init {
  FilterPipelineState filterPipelineState;
  filterPipelineState.filterNameStr = @"null filter";
  filterPipelineState.fragmentFuncNameStr = @"redYuv420spFragmentShader";
  filterPipelineState.vertexFuncNameStr = @"redVertexShader";
  filterPipelineState.computeFuncNameStr = @"computeShader";
  filterPipelineState.depthPixelFormat = MTLPixelFormatDepth32Float;
  filterPipelineState.stencilPixelFormat = MTLPixelFormatInvalid;
  filterPipelineState.inputTexNum = 1;
  filterPipelineState.sampleCount = 1;
  filterPipelineState.depthWriteEnabled = NO;
  filterPipelineState.pixelFormat = 0;

  int sessionID = 0;
  if (![self initWithFilterPipelineState:nil
                     filterPipelineState:&filterPipelineState
                               sessionID:(int)sessionID] ||
      !_filterDevice) {
    return nil;
  }
  return self;
}

- (void)dealloc {
  inputTextures = nil;
  inputTextureIndexs = nil;
  inputTexturesRotationMode = nil;
  computePipelineState = nil;
  renderPipelineState = nil;
  renderPipelineStateDescriptor = nil;
  renderPassDescriptor = nil;
  depthStencilState = nil;
  renderDepthStateDesc = nil;

  _mMetalVideoCmdQueue = nil;
  _filterDevice = nil;
  _filterLibrary = nil;
  _verticsCoordinateBuffer = nil;
  _textureCoordinateBuffer = nil;
  _depthTexture = nil;
  _stencilTexture = nil;
  _msaaTexture = nil;
  _filterDrawable = nil;
}

- (id)initWithFilterPipelineState:(MetalVideoCmdQueue *)metalVideoCmdQueue
              filterPipelineState:(FilterPipelineState *)filterPipelineState
                        sessionID:(int)sessionID {
  if (!(self = [super init])) {
    return nil;
  }
  _sessionID = sessionID;
  if (!metalVideoCmdQueue) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] metalVideoCmdQueue==nil error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }
  _mMetalVideoCmdQueue = metalVideoCmdQueue;
  if (!_filterDevice) {
    _filterDevice = metalVideoCmdQueue.device;
  }

  NSString *libraryFile = [[NSBundle bundleForClass:[self class]]
      pathForResource:@"RedRenderShaders"
               ofType:@"metallib"];
  if (libraryFile.length == 0) {
    NSLog(@"[redrender]:[id @ %d] [%s:%d] pathForResource error .\n",
          _sessionID, __FUNCTION__, __LINE__);
    return nil;
  }
  NSError *error = nil;
  _filterLibrary = [_filterDevice newLibraryWithFile:libraryFile error:&error];
  if (!_filterLibrary) {
    NSLog(@"[redrender]:[id @ %d] [%s:%d] newLibraryWithFile error:%@ .\n",
          _sessionID, __FUNCTION__, __LINE__, error);
    return nil;
  }

  _filterNameStr = filterPipelineState->filterNameStr;
  _metalFilterType = filterPipelineState->metalFilterType;
  fragmentFuncNameStr = filterPipelineState->fragmentFuncNameStr;
  vertexFuncNameStr = filterPipelineState->vertexFuncNameStr;
  computeFuncNameStr = filterPipelineState->computeFuncNameStr;

  _depthPixelFormat = filterPipelineState->depthPixelFormat;
  _stencilPixelFormat = filterPipelineState->stencilPixelFormat;
  _sampleCount = filterPipelineState->sampleCount;
  _inputTexNum = filterPipelineState->inputTexNum;
  _depthWriteEnabled = filterPipelineState->depthWriteEnabled;
  _pixelFormat = filterPipelineState->pixelFormat;
  _paddingPixels = 0;

  inputTextures =
      [[NSPointerArray alloc] initWithOptions:NSPointerFunctionsStrongMemory];
  inputTextureIndexs = [[NSMutableArray alloc] init];
  inputTexturesRotationMode = [[NSMutableArray alloc] init];
  outputTexture = nil;
  sharedcmdBuffer = nil;

  if (![self initPipelineState:filterPipelineState sessionID:(int)sessionID]) {
    return nil;
  }

  if ((_depthPixelFormat != MTLPixelFormatInvalid ||
       _stencilPixelFormat != MTLPixelFormatInvalid) &&
      ![self initDepthStencilState]) {
    return nil;
  }

  return self;
}

- (BOOL)initPipelineState:(FilterPipelineState *)filterPipelineState
                sessionID:(int)sessionID {
  NSError *error = nil;
  if (_filterDevice && !_filterLibrary) {
    NSString *libraryFile = [[NSBundle bundleForClass:[self class]]
        pathForResource:@"RedRenderShaders"
                 ofType:@"metallib"];
    if (libraryFile.length == 0) {
      NSLog(@"[redrender]:[id @ %d] [%s:%d] pathForResource error .\n",
            _sessionID, __FUNCTION__, __LINE__);
      return NO;
    }
    NSError *error = nil;
    id<MTLLibrary> defaultLibrary =
        [_filterDevice newLibraryWithFile:libraryFile error:&error];
    ;
    if (!defaultLibrary) {
      NSLog(@"[redrender]:[id @ %d] [%s:%d] newLibraryWithFile error:%@ .\n",
            _sessionID, __FUNCTION__, __LINE__, error);
      return NO;
    }
  }

  if (![filterPipelineState->fragmentFuncNameStr isEqualToString:@""] &&
      ![filterPipelineState->vertexFuncNameStr isEqualToString:@""]) {
    id<MTLFunction> fragmentFunc = [_filterLibrary
        newFunctionWithName:filterPipelineState->fragmentFuncNameStr];
    id<MTLFunction> vertexFunc = [_filterLibrary
        newFunctionWithName:filterPipelineState->vertexFuncNameStr];
    if (!fragmentFunc || !vertexFunc) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] create function error .\n",
                 __FUNCTION__, __LINE__);
      return NO;
    }
    renderPipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
    if (!renderPipelineStateDescriptor) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] create renderPipelineStateDescriptor error .\n",
                 __FUNCTION__, __LINE__);
      return NO;
    }
    renderPipelineStateDescriptor.depthAttachmentPixelFormat =
        filterPipelineState->depthPixelFormat;
    renderPipelineStateDescriptor.stencilAttachmentPixelFormat =
        filterPipelineState->stencilPixelFormat;
    renderPipelineStateDescriptor.colorAttachments[0].pixelFormat =
        MTLPixelFormatBGRA8Unorm;
    renderPipelineStateDescriptor.sampleCount =
        filterPipelineState->sampleCount;
    renderPipelineStateDescriptor.fragmentFunction = fragmentFunc;
    renderPipelineStateDescriptor.vertexFunction = vertexFunc;

    MTLRenderPipelineColorAttachmentDescriptor *attachmentDesc =
        renderPipelineStateDescriptor.colorAttachments[0];

    attachmentDesc.blendingEnabled = true;
    attachmentDesc.rgbBlendOperation = MTLBlendOperationAdd;
    attachmentDesc.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    attachmentDesc.destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;

    attachmentDesc.alphaBlendOperation = MTLBlendOperationAdd;
    attachmentDesc.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    attachmentDesc.destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;

    renderPipelineState = [_filterDevice
        newRenderPipelineStateWithDescriptor:renderPipelineStateDescriptor
                                       error:&error];

    if (!renderPipelineState) {
      NSLog(@"[redrender]:[id @ %d] [%s:%d] create renderPipelineState "
            @"error:%@ .\n",
            _sessionID, __FUNCTION__, __LINE__, error);
      return NO;
    }
  }

  if (![filterPipelineState->computeFuncNameStr isEqualToString:@""]) {
    id<MTLFunction> computeFunc = [_filterLibrary
        newFunctionWithName:filterPipelineState->computeFuncNameStr];
    if (!computeFunc) {
      AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] create function error .\n",
                 __FUNCTION__, __LINE__);
      return NO;
    }

    computePipelineState =
        [_filterDevice newComputePipelineStateWithFunction:computeFunc
                                                     error:&error];
    if (!computePipelineState) {
      NSLog(@"[redrender]:[id @ %d] [%s:%d] create computePipelineState  "
            @"error:%@ .\n",
            _sessionID, __FUNCTION__, __LINE__, error);
      return NO;
    }

    NSInteger w = computePipelineState.threadExecutionWidth;
    NSInteger h = computePipelineState.maxTotalThreadsPerThreadgroup / w;
    threadsPerThreadgroup = MTLSizeMake(w, h, 1);
  }
  return YES;
}

- (BOOL)initDepthStencilState {
  MTLDepthStencilDescriptor *depthStateDsc = [MTLDepthStencilDescriptor new];
  if (!depthStateDsc) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] MTLDepthStencilDescriptor error.\n", __FUNCTION__,
               __LINE__);
    return NO;
  }

  depthStateDsc.depthCompareFunction = MTLCompareFunctionAlways;
  depthStateDsc.depthWriteEnabled = _depthWriteEnabled;
  depthStencilState =
      [_filterDevice newDepthStencilStateWithDescriptor:depthStateDsc];
  if (!depthStencilState) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] newDepthStencilStateWithDescriptor error.\n",
               __FUNCTION__, __LINE__);
    return NO;
  }

  return YES;
}

- (void)onRender:(RedRenderMetalView *)view {
}

- (BOOL)renderWithEncoder:(id<MTLRenderCommandEncoder>)renderEncoder {
  return YES;
}

- (BOOL)updateParam:(RedRenderMetalView *)view {
  if (inputTextures.count == 0) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] inputTextures empty error .\n",
               __FUNCTION__, __LINE__);
    return NO;
  }
  if (nullptr == view) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] view is nil.\n", __FUNCTION__,
               __LINE__);
    return NO;
  }

  modelViewProjectionMatrix = getModelViewProjectionMatrix();

  MetalVideoTexture *firstInputTexture =
      (MetalVideoTexture *)[inputTextures pointerAtIndex:0];
  if (!firstInputTexture) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] inputTextures empty error .\n",
               __FUNCTION__, __LINE__);
    return NO;
  }

  int rotatedTextureWidth = firstInputTexture.width;
  int rotatedTextureHeight = firstInputTexture.height;
  if (rotationSwapsSize(
          (RedRender::RotationMode)firstInputTexture.rotationMode)) {
    rotatedTextureWidth = firstInputTexture.height;
    rotatedTextureHeight = firstInputTexture.width;
    if (_metalFilterType == RedRender::VideoMetalDeviceFilterType) {
      _inputFrameMetaData->frameWidth = rotatedTextureWidth;
      _inputFrameMetaData->frameHeight = rotatedTextureHeight;
    }
  }
  if (outputTextureScale != 1.0) {
    rotatedTextureWidth = int(rotatedTextureWidth * outputTextureScale);
    rotatedTextureHeight = int(rotatedTextureHeight * outputTextureScale);
    if (_metalFilterType == RedRender::VideoMetalDeviceFilterType) {
      _inputFrameMetaData->frameWidth = rotatedTextureWidth;
      _inputFrameMetaData->frameHeight = rotatedTextureHeight;
    }
  }
  if (_metalFilterType == RedRender::VideoMetalDeviceFilterType) {
    if (_inputFrameMetaData->linesize[0] > _inputFrameMetaData->frameWidth) {
      _paddingPixels =
          _inputFrameMetaData->linesize[0] - _inputFrameMetaData->frameWidth;
    }
  }

  if (![computeFuncNameStr isEqualToString:@""]) {
    if (threadsPerThreadgroup.width == 0 || threadsPerThreadgroup.height == 0 ||
        threadsPerThreadgroup.depth == 0) {
      NSInteger w = computePipelineState.threadExecutionWidth;
      NSInteger h = computePipelineState.maxTotalThreadsPerThreadgroup / w;
      threadsPerThreadgroup = MTLSizeMake(w, h, 1);
    }

    threadsPerGrid = MTLSizeMake(rotatedTextureWidth, rotatedTextureHeight, 1);

    NSUInteger nthreadWidthSteps =
        (rotatedTextureWidth + threadsPerThreadgroup.width - 1) /
        threadsPerThreadgroup.width;
    NSUInteger nthreadHeightSteps =
        (rotatedTextureHeight + threadsPerThreadgroup.height - 1) /
        threadsPerThreadgroup.height;
    threadgroupsPerGrid = MTLSizeMake(nthreadWidthSteps, nthreadHeightSteps, 1);
  }

  if (![fragmentFuncNameStr isEqualToString:@""] &&
      ![vertexFuncNameStr isEqualToString:@""]) {
    // render screen
    if (_metalFilterType == RedRender::VideoMetalDeviceFilterType) {
      CGSize drawableSize = view.mDrawableSize;
      _inputFrameMetaData->layerWidth = drawableSize.width;
      _inputFrameMetaData->layerHeight = drawableSize.height;
      _filterDrawable = [view getCurrentDrawable];
      if (!_filterDrawable || !_filterDrawable.texture) {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] get a _filterDrawable error .\n", __FUNCTION__,
                   __LINE__);
        renderPassDescriptor = nil;
        return NO;
      }
      [self initRenderPassDescriptor:_filterDrawable.texture];
      _filterDrawable = nil;
    } else { // post-render processing
      if (outputTexture == nil || outputTexture.width != rotatedTextureWidth ||
          outputTexture.height != rotatedTextureHeight) {
        if (outputTexture) {
          outputTexture = nil;
        }
        outputTexture =
            [[MetalVideoTexture alloc] init:_filterDevice
                                  sessionID:_sessionID
                                      width:rotatedTextureWidth
                                     height:rotatedTextureHeight
                                pixelFormat:MTLPixelFormatBGRA8Unorm];
      }
      if (outputTexture) {
        [self initRenderPassDescriptor:outputTexture.texture];
      } else {
        AV_LOGE_ID(LOG_TAG, _sessionID,
                   "[%s:%d] MetalVideoTexture alloc error .\n", __FUNCTION__,
                   __LINE__);
        renderPassDescriptor = nil;
      }
    }
  }

  if (![computeFuncNameStr isEqualToString:@""]) {
    if (outputTexture == nil || outputTexture.width != rotatedTextureWidth ||
        outputTexture.height != rotatedTextureHeight) {
      if (outputTexture) {
        outputTexture = nil;
      }
      outputTexture = [[MetalVideoTexture alloc] init:_filterDevice
                                            sessionID:_sessionID
                                                width:rotatedTextureWidth
                                               height:rotatedTextureHeight
                                          pixelFormat:MTLPixelFormatBGRA8Unorm];
    }
    if (!outputTexture) {
      AV_LOGE_ID(LOG_TAG, _sessionID,
                 "[%s:%d] MetalVideoTexture alloc error .\n", __FUNCTION__,
                 __LINE__);
    }
  }
  return YES;
}

- (void)initRenderPassDescriptor:(id<MTLTexture>)texture {
  if (!texture) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] texture==nil error .\n",
               __FUNCTION__, __LINE__);
    return;
  }
  if (!renderPassDescriptor) {
    renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
  }
  MTLRenderPassColorAttachmentDescriptor *colorAttachment =
      renderPassDescriptor.colorAttachments[0];
  colorAttachment.loadAction = MTLLoadActionClear;
  colorAttachment.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
  if (_sampleCount > 1) {
    if (!_msaaTexture || (_msaaTexture.width != texture.width) ||
        (_msaaTexture.height != texture.height) ||
        (_msaaTexture.sampleCount != _sampleCount)) {
      _msaaTexture = nil;
      MTLTextureDescriptor *desc = [MTLTextureDescriptor
          texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                       width:texture.width
                                      height:texture.height
                                   mipmapped:NO];
      desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite |
                   MTLTextureUsageRenderTarget;
      desc.textureType = MTLTextureType2DMultisample;
      desc.sampleCount = self.sampleCount;
      _msaaTexture = [_filterDevice newTextureWithDescriptor:desc];
    }
    colorAttachment.texture = _msaaTexture;
    colorAttachment.resolveTexture = texture;
    colorAttachment.storeAction = MTLStoreActionMultisampleResolve;
  } else {
    colorAttachment.texture = texture;
    colorAttachment.storeAction = MTLStoreActionStore;
  }

  if (_depthPixelFormat != MTLPixelFormatInvalid) {
    if (!_depthTexture || (_depthTexture.width != texture.width) ||
        (_depthTexture.height != texture.height) ||
        (_depthTexture.sampleCount != _sampleCount)) {

      MTLTextureDescriptor *desc = [MTLTextureDescriptor
          texture2DDescriptorWithPixelFormat:_depthPixelFormat
                                       width:texture.width
                                      height:texture.height
                                   mipmapped:NO];
      desc.textureType =
          (_sampleCount > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
      desc.sampleCount = _sampleCount;
      desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite |
                   MTLTextureUsageRenderTarget;
      _depthTexture = [_filterDevice newTextureWithDescriptor:desc];

      MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
          renderPassDescriptor.depthAttachment;
      depthAttachment.texture = _depthTexture;
      depthAttachment.loadAction = MTLLoadActionClear;
      depthAttachment.storeAction = MTLStoreActionDontCare;
      depthAttachment.clearDepth = 1.0;
    }
  }

  if (_stencilPixelFormat != MTLPixelFormatInvalid) {
    if (!_stencilTexture || (_stencilTexture.width != texture.width) ||
        (_stencilTexture.height != texture.height) ||
        (_stencilTexture.sampleCount != _sampleCount)) {

      MTLTextureDescriptor *desc = [MTLTextureDescriptor
          texture2DDescriptorWithPixelFormat:_stencilPixelFormat
                                       width:texture.width
                                      height:texture.height
                                   mipmapped:NO];
      desc.textureType =
          (_sampleCount > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
      desc.sampleCount = _sampleCount;
      _stencilTexture = [_filterDevice newTextureWithDescriptor:desc];

      MTLRenderPassStencilAttachmentDescriptor *stencilAttachment =
          renderPassDescriptor.stencilAttachment;
      stencilAttachment.texture = _stencilTexture;
      stencilAttachment.loadAction = MTLLoadActionClear;
      stencilAttachment.storeAction = MTLStoreActionDontCare;
      stencilAttachment.clearStencil = 0;
    }
  }
}

- (void)initVerticsCoordinateBuffer {
  if (_verticsCoordinateBuffer) {
    _verticsCoordinateBuffer = nil;
  }

  [self verticesCoordinateReset];
  if (_metalFilterType != RedRender::VideoMetalDeviceFilterType) {
    const simd::float4 vertices[] = {
        {verticesCoordinate[0], verticesCoordinate[1], 0.0f, 1.0f},
        {verticesCoordinate[2], verticesCoordinate[3], 0.0f, 1.0f},
        {verticesCoordinate[4], verticesCoordinate[5], 0.0f, 1.0f},
        {verticesCoordinate[2], verticesCoordinate[3], 0.0f, 1.0f},
        {verticesCoordinate[4], verticesCoordinate[5], 0.0f, 1.0f},
        {verticesCoordinate[6], verticesCoordinate[7], 0.0f, 1.0f},
    };
    _verticsCoordinateBuffer =
        [_filterDevice newBufferWithBytes:vertices
                                   length:sizeof(vertices)
                                  options:MTLResourceOptionCPUCacheModeDefault];
  } else {
    float frameWidth = _inputFrameMetaData->frameWidth;
    float frameHeight = _inputFrameMetaData->frameHeight;
    float x_Value = 1.0f;
    float y_Value = 1.0f;

    switch (_inputFrameMetaData->aspectRatio) {
    case RedRender::AspectRatioEqualWidth: {
      if (_inputFrameMetaData->sar.num > 0 &&
          _inputFrameMetaData->sar.den > 0) {
        frameHeight = frameHeight * _inputFrameMetaData->sar.den /
                      _inputFrameMetaData->sar.num;
      }
      y_Value =
          ((frameHeight * 1.0f / frameWidth) * _inputFrameMetaData->layerWidth /
           _inputFrameMetaData->layerHeight);
      verticesCoordinate[1] = -1.0f * y_Value;
      verticesCoordinate[3] = -1.0f * y_Value;
      verticesCoordinate[5] = 1.0f * y_Value;
      verticesCoordinate[7] = 1.0f * y_Value;
    } break;
    case RedRender::AspectRatioEqualHeight: {
      if (_inputFrameMetaData->sar.num > 0 &&
          _inputFrameMetaData->sar.den > 0) {
        frameWidth = frameWidth * _inputFrameMetaData->sar.num /
                     _inputFrameMetaData->sar.den;
      }
      x_Value =
          ((frameWidth * 1.0f / frameHeight) *
           _inputFrameMetaData->layerHeight / _inputFrameMetaData->layerWidth);
      verticesCoordinate[0] = -1.0f * x_Value;
      verticesCoordinate[2] = 1.0f * x_Value;
      verticesCoordinate[4] = -1.0f * x_Value;
      verticesCoordinate[6] = 1.0f * x_Value;
    } break;
    case RedRender::ScaleAspectFit: {
      if (_inputFrameMetaData->sar.num > 0 &&
          _inputFrameMetaData->sar.den > 0) {
        frameWidth = frameWidth * _inputFrameMetaData->sar.num /
                     _inputFrameMetaData->sar.den;
      }
      const CGFloat dW = (CGFloat)_inputFrameMetaData->layerWidth / frameWidth;
      const CGFloat dH =
          (CGFloat)_inputFrameMetaData->layerHeight / frameHeight;

      CGFloat dd = 1.0f;
      CGFloat nW = 1.0f;
      CGFloat nH = 1.0f;
      dd = std::fmin(dW, dH);

      nW = (frameWidth * dd / (CGFloat)_inputFrameMetaData->layerWidth);
      nH = (frameHeight * dd / (CGFloat)_inputFrameMetaData->layerHeight);

      verticesCoordinate[0] = -nW;
      verticesCoordinate[1] = -nH;
      verticesCoordinate[2] = nW;
      verticesCoordinate[3] = -nH;
      verticesCoordinate[4] = -nW;
      verticesCoordinate[5] = nH;
      verticesCoordinate[6] = nW;
      verticesCoordinate[7] = nH;
    } break;
    case RedRender::ScaleAspectFill: {
      if (_inputFrameMetaData->sar.num > 0 &&
          _inputFrameMetaData->sar.den > 0) {
        frameWidth = frameWidth * _inputFrameMetaData->sar.num /
                     _inputFrameMetaData->sar.den;
      }
      const CGFloat dW = (CGFloat)_inputFrameMetaData->layerWidth / frameWidth;
      const CGFloat dH =
          (CGFloat)_inputFrameMetaData->layerHeight / frameHeight;

      CGFloat dd = 1.0f;
      CGFloat nW = 1.0f;
      CGFloat nH = 1.0f;
      dd = std::fmax(dW, dH);

      nW = (frameWidth * dd / (CGFloat)_inputFrameMetaData->layerWidth);
      nH = (frameHeight * dd / (CGFloat)_inputFrameMetaData->layerHeight);

      verticesCoordinate[0] = -nW;
      verticesCoordinate[1] = -nH;
      verticesCoordinate[2] = nW;
      verticesCoordinate[3] = -nH;
      verticesCoordinate[4] = -nW;
      verticesCoordinate[5] = nH;
      verticesCoordinate[6] = nW;
      verticesCoordinate[7] = nH;
    } break;
    case RedRender::AspectRatioFill:
    default:
      break;
    }
    const simd::float4 vertices[] = {
        {verticesCoordinate[0], verticesCoordinate[1], 0.0, 1.0},
        {verticesCoordinate[2], verticesCoordinate[3], 0.0, 1.0},
        {verticesCoordinate[4], verticesCoordinate[5], 0.0, 1.0},
        {verticesCoordinate[2], verticesCoordinate[3], 0.0, 1.0},
        {verticesCoordinate[4], verticesCoordinate[5], 0.0, 1.0},
        {verticesCoordinate[6], verticesCoordinate[7], 0.0, 1.0},
    };
    _verticsCoordinateBuffer =
        [_filterDevice newBufferWithBytes:vertices
                                   length:sizeof(vertices)
                                  options:MTLResourceOptionCPUCacheModeDefault];
  }

  if (!_verticsCoordinateBuffer) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] create _verticsCoordinateBuffer error .\n",
               __FUNCTION__, __LINE__);
  }
}

- (void)verticesCoordinateReset {
  verticesCoordinate[0] = -1.0f;
  verticesCoordinate[1] = -1.0f;
  verticesCoordinate[2] = 1.0f;
  verticesCoordinate[3] = -1.0f;
  verticesCoordinate[4] = -1.0f;
  verticesCoordinate[5] = 1.0f;
  verticesCoordinate[6] = 1.0f;
  verticesCoordinate[7] = 1.0f;
}

- (void)initTexureCoordinateBuffer:(RedRender::RotationMode)rotationMode {
  if (_textureCoordinateBuffer) {
    _textureCoordinateBuffer = nil;
  }

  [self texureCoordinateReset:rotationMode];
  if (_paddingPixels > 0) {
    GLfloat cropSize =
        ((GLfloat)_paddingPixels) / _inputFrameMetaData->linesize[0];
    [self texureCoordinateCrop:rotationMode cropSize:cropSize];
  }

  const simd::float2 vertices1[] = {
      {texureCoordinate[0], texureCoordinate[1]},
      {texureCoordinate[2], texureCoordinate[3]},
      {texureCoordinate[4], texureCoordinate[5]},
      {texureCoordinate[2], texureCoordinate[3]},
      {texureCoordinate[4], texureCoordinate[5]},
      {texureCoordinate[6], texureCoordinate[7]},
  };
  _textureCoordinateBuffer =
      [_filterDevice newBufferWithBytes:vertices1
                                 length:sizeof(vertices1)
                                options:MTLResourceOptionCPUCacheModeDefault];
  if (!_textureCoordinateBuffer) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] create _textureCoordinateBuffer error .\n",
               __FUNCTION__, __LINE__);
  }
}

- (void)texureCoordinateReset:(const RedRender::RotationMode &)rotationMode {
  switch (rotationMode) {
  case RedRender::RotateLeft:
    texureCoordinate[0] = 0.0f;
    texureCoordinate[1] = 0.0f;
    texureCoordinate[2] = 0.0f;
    texureCoordinate[3] = 1.0f;
    texureCoordinate[4] = 1.0f;
    texureCoordinate[5] = 0.0f;
    texureCoordinate[6] = 1.0f;
    texureCoordinate[7] = 1.0f;
    break;
  case RedRender::RotateRight:
    texureCoordinate[0] = 1.0f;
    texureCoordinate[1] = 1.0f;
    texureCoordinate[2] = 1.0f;
    texureCoordinate[3] = 0.0f;
    texureCoordinate[4] = 0.0f;
    texureCoordinate[5] = 1.0f;
    texureCoordinate[6] = 0.0f;
    texureCoordinate[7] = 0.0f;
    break;
  case RedRender::FlipVertical:
    texureCoordinate[0] = 0.0f;
    texureCoordinate[1] = 0.0f;
    texureCoordinate[2] = 1.0f;
    texureCoordinate[3] = 0.0f;
    texureCoordinate[4] = 0.0f;
    texureCoordinate[5] = 1.0f;
    texureCoordinate[6] = 1.0f;
    texureCoordinate[7] = 1.0f;
    break;
  case RedRender::FlipHorizontal:
    texureCoordinate[0] = 1.0f;
    texureCoordinate[1] = 1.0f;
    texureCoordinate[2] = 0.0f;
    texureCoordinate[3] = 1.0f;
    texureCoordinate[4] = 1.0f;
    texureCoordinate[5] = 0.0f;
    texureCoordinate[6] = 0.0f;
    texureCoordinate[7] = 0.0f;
    break;
  case RedRender::RotateRightFlipVertical:
    texureCoordinate[0] = 1.0f;
    texureCoordinate[1] = 0.0f;
    texureCoordinate[2] = 1.0f;
    texureCoordinate[3] = 1.0f;
    texureCoordinate[4] = 0.0f;
    texureCoordinate[5] = 0.0f;
    texureCoordinate[6] = 0.0f;
    texureCoordinate[7] = 1.0f;
    break;
  case RedRender::RotateRightFlipHorizontal:
    texureCoordinate[0] = 0.0f;
    texureCoordinate[1] = 1.0f;
    texureCoordinate[2] = 0.0f;
    texureCoordinate[3] = 0.0f;
    texureCoordinate[4] = 1.0f;
    texureCoordinate[5] = 1.0f;
    texureCoordinate[6] = 1.0f;
    texureCoordinate[7] = 0.0f;
    break;
  case RedRender::Rotate180:
    texureCoordinate[0] = 1.0f;
    texureCoordinate[1] = 0.0f;
    texureCoordinate[2] = 0.0f;
    texureCoordinate[3] = 0.0f;
    texureCoordinate[4] = 1.0f;
    texureCoordinate[5] = 1.0f;
    texureCoordinate[6] = 0.0f;
    texureCoordinate[7] = 1.0f;
    break;
  case RedRender::NoRotation:
  default:
    texureCoordinate[0] = 0.0f;
    texureCoordinate[1] = 1.0f;
    texureCoordinate[2] = 1.0f;
    texureCoordinate[3] = 1.0f;
    texureCoordinate[4] = 0.0f;
    texureCoordinate[5] = 0.0f;
    texureCoordinate[6] = 1.0f;
    texureCoordinate[7] = 0.0f;
    break;
  }
}

- (void)texureCoordinateCrop:(RedRender::RotationMode)rotationMode
                    cropSize:(GLfloat)cropSize {
  switch (rotationMode) {
  case RedRender::RotateLeft:
    texureCoordinate[4] = texureCoordinate[4] - cropSize;
    texureCoordinate[6] = texureCoordinate[6] - cropSize;
    break;
  case RedRender::RotateRight:
    texureCoordinate[0] = texureCoordinate[0] - cropSize;
    texureCoordinate[2] = texureCoordinate[2] - cropSize;
    break;
  case RedRender::FlipVertical:
    texureCoordinate[2] = texureCoordinate[2] - cropSize;
    texureCoordinate[6] = texureCoordinate[6] - cropSize;
    break;
  case RedRender::FlipHorizontal:
    texureCoordinate[0] = texureCoordinate[0] - cropSize;
    texureCoordinate[4] = texureCoordinate[4] - cropSize;
    break;
  case RedRender::RotateRightFlipVertical:
    texureCoordinate[0] = texureCoordinate[0] - cropSize;
    texureCoordinate[2] = texureCoordinate[2] - cropSize;
    break;
  case RedRender::RotateRightFlipHorizontal:
    texureCoordinate[4] = texureCoordinate[4] - cropSize;
    texureCoordinate[6] = texureCoordinate[6] - cropSize;
    break;
  case RedRender::Rotate180:
    texureCoordinate[0] = texureCoordinate[0] - cropSize;
    texureCoordinate[4] = texureCoordinate[4] - cropSize;
    break;
  case RedRender::NoRotation:
  default:
    texureCoordinate[2] = texureCoordinate[2] - cropSize;
    if (_metalFilterType == RedRender::VideoMetalDeviceFilterType &&
        _inputFrameMetaData->pixel_format !=
            RedRender::VRPixelFormatYUV420sp_vtb &&
        texureCoordinate[2] > 0 && texureCoordinate[6] > 0) {
      uint64_t tenThousands = 0;
      tenThousands = (uint64_t)(floorf(texureCoordinate[2] * 10000)) % 10;
      if (tenThousands > 5) {
        texureCoordinate[2] = floorf(texureCoordinate[2] * 1000.0) / 1000.0;
      } else {
        texureCoordinate[2] =
            floorf(texureCoordinate[2] * 1000.0) / 1000.0 - 0.001;
      }
    }
    texureCoordinate[6] = texureCoordinate[6] - cropSize;
    if (_metalFilterType == RedRender::VideoMetalDeviceFilterType &&
        _inputFrameMetaData->pixel_format !=
            RedRender::VRPixelFormatYUV420sp_vtb &&
        texureCoordinate[2] > 0 && texureCoordinate[6] > 0) {
      uint64_t tenThousands = 0;
      tenThousands = (uint64_t)(floorf(texureCoordinate[6] * 10000)) % 10;
      if (tenThousands > 5) {
        texureCoordinate[6] = floorf(texureCoordinate[6] * 1000.0) / 1000.0;
      } else {
        texureCoordinate[6] =
            floorf(texureCoordinate[6] * 1000.0) / 1000.0 - 0.001;
      }
    }

    break;
  }
}

#pragma mark MetalVideoInput

- (void)setInputTexture:(MetalVideoTexture *)inputTexture
           rotationMode:(RedRender::RotationMode)rotationMode
      inputTextureIndex:(int)inputTextureIndex {
  if ([inputTextureIndexs
          containsObject:[NSNumber numberWithInt:inputTextureIndex]]) {
    NSInteger indexOfObject = [inputTextureIndexs
        indexOfObject:[NSNumber numberWithInteger:inputTextureIndex]];
    inputTexture.inputTextureIndex = inputTextureIndex;
    inputTexture.rotationMode = rotationMode;
    [inputTextures replacePointerAtIndex:indexOfObject
                             withPointer:(void *)inputTexture];
    return;
  }

  inputTexture.inputTextureIndex = inputTextureIndex;
  inputTexture.rotationMode = rotationMode;

  [inputTextures addPointer:(void *)inputTexture];
  [inputTextureIndexs addObject:[NSNumber numberWithInt:inputTextureIndex]];
  [inputTexturesRotationMode addObject:[NSNumber numberWithInt:rotationMode]];
}

- (void)clearInputTexture {
  int count = (int)inputTextures.count;
  for (int i = 0; i < count; i++) {
    [inputTextures removePointerAtIndex:0];
  }
  [inputTextureIndexs removeAllObjects];
  [inputTexturesRotationMode removeAllObjects];
}

- (bool)isPrepared {
  if (inputTextures.count >= _inputTexNum &&
      inputTextureIndexs.count >= _inputTexNum &&
      inputTexturesRotationMode.count >= _inputTexNum) {
    return YES;
  }
  return NO;
}

@end
#endif
