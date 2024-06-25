/*
 * metal_filter_device.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_filter_device.h"
#import "metal_common.h"

@implementation MetalFilterDevice {
  matrix_float3x3 _colorConversionMatrix;
}

- (id)init:(MetalVideoCmdQueue *)metalVideoCmdQueue
    inputFrameMetaData:(RedRender::VideoFrameMetaData *)inputFrameMetaData
             sessionID:(int)sessionID {
  FilterPipelineState filterPipelineState;
  filterPipelineState.filterNameStr = @"MetalDeviceFilter";
  filterPipelineState.metalFilterType = RedRender::VideoMetalDeviceFilterType;
  filterPipelineState.computeFuncNameStr = @"";
  filterPipelineState.depthPixelFormat =
      MTLPixelFormatInvalid; // MTLPixelFormatDepth32Float;
  filterPipelineState.stencilPixelFormat = MTLPixelFormatInvalid;
  filterPipelineState.sampleCount = 1;
  filterPipelineState.depthWriteEnabled = NO;
  filterPipelineState.pixelFormat = inputFrameMetaData->pixel_format;

  switch (inputFrameMetaData->pixel_format) {
  case RedRender::VRPixelFormatRGB565:
  case RedRender::VRPixelFormatRGB888:
  case RedRender::VRPixelFormatRGBX8888:
    filterPipelineState.inputTexNum = 1;
    filterPipelineState.fragmentFuncNameStr = @"redRgbFragmentShader";
    filterPipelineState.vertexFuncNameStr = @"redVertexShader";
    if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                filterPipelineState:&filterPipelineState
                                          sessionID:sessionID]) ||
        !self.filterDevice) {
      AV_LOGE_ID(LOG_TAG, sessionID,
                 "[%s:%d] initWithFilterPipelineState error .\n", __FUNCTION__,
                 __LINE__);
      return nil;
    }
    break;
  case RedRender::VRPixelFormatYUV420p: {
    filterPipelineState.inputTexNum = 3;
    if (inputFrameMetaData->color_range == RedRender::VR_AVCOL_RANGE_JPEG) {
      filterPipelineState.fragmentFuncNameStr =
          @"redYuv420pFullRangeFragmentShader";
      filterPipelineState.vertexFuncNameStr = @"redVertexShader";

      if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                  filterPipelineState:&filterPipelineState
                                            sessionID:sessionID]) ||
          !self.filterDevice) {
        AV_LOGE_ID(LOG_TAG, sessionID,
                   "[%s:%d] initWithFilterPipelineState error .\n",
                   __FUNCTION__, __LINE__);
        return nil;
      }
    } else {
      filterPipelineState.fragmentFuncNameStr = @"redYuv420pFragmentShader";
      filterPipelineState.vertexFuncNameStr = @"redVertexShader";

      if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                  filterPipelineState:&filterPipelineState
                                            sessionID:sessionID]) ||
          !self.filterDevice) {
        AV_LOGE_ID(LOG_TAG, sessionID,
                   "[%s:%d] initWithFilterPipelineState error .\n",
                   __FUNCTION__, __LINE__);
        return nil;
      }
    }
  } break;
  case RedRender::VRPixelFormatYUV420sp: {
    filterPipelineState.inputTexNum = 2;
    if (inputFrameMetaData->color_range == RedRender::VR_AVCOL_RANGE_JPEG) {
      filterPipelineState.fragmentFuncNameStr =
          @"redYuv420spFullRangeFragmentShader";
      filterPipelineState.vertexFuncNameStr = @"redVertexShader";
      if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                  filterPipelineState:&filterPipelineState
                                            sessionID:sessionID]) ||
          !self.filterDevice) {
        AV_LOGE_ID(LOG_TAG, sessionID,
                   "[%s:%d] initWithFilterPipelineState error .\n",
                   __FUNCTION__, __LINE__);
        return nil;
      }
    } else {
      filterPipelineState.fragmentFuncNameStr = @"redYuv420spFragmentShader";
      filterPipelineState.vertexFuncNameStr = @"redVertexShader";
      if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                  filterPipelineState:&filterPipelineState
                                            sessionID:sessionID]) ||
          !self.filterDevice) {
        AV_LOGE_ID(LOG_TAG, sessionID,
                   "[%s:%d] initWithFilterPipelineState error .\n",
                   __FUNCTION__, __LINE__);
        return nil;
      }
    }
  } break;
  case RedRender::VRPixelFormatYUV420sp_vtb: {
    filterPipelineState.inputTexNum = 2;
    if (inputFrameMetaData->color_range == RedRender::VR_AVCOL_RANGE_JPEG) {
      filterPipelineState.fragmentFuncNameStr =
          @"redYuv420spFullRangeFragmentShader";
      filterPipelineState.vertexFuncNameStr = @"redVertexShader";

      if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                  filterPipelineState:&filterPipelineState
                                            sessionID:sessionID]) ||
          !self.filterDevice) {
        AV_LOGE_ID(LOG_TAG, sessionID,
                   "[%s:%d] initWithFilterPipelineState error .\n",
                   __FUNCTION__, __LINE__);
        return nil;
      }
    } else {
      filterPipelineState.fragmentFuncNameStr = @"redYuv420spFragmentShader";
      filterPipelineState.vertexFuncNameStr = @"redVertexShader";
      if (!(self = [super initWithFilterPipelineState:metalVideoCmdQueue
                                  filterPipelineState:&filterPipelineState
                                            sessionID:sessionID]) ||
          !self.filterDevice) {
        AV_LOGE_ID(LOG_TAG, sessionID,
                   "[%s:%d] initWithFilterPipelineState error .\n",
                   __FUNCTION__, __LINE__);
        return nil;
      }
    }
  } break;
  default:
    break;
  }
  AV_LOGI_ID(LOG_TAG, sessionID,
             "[%s:%d] filter=MetalDeviceFilter create success .\n",
             __FUNCTION__, __LINE__);

  return self;
}

- (void)onRender:(RedRenderMetalView *)view {
  if (sharedcmdBuffer && renderPipelineState &&
      renderPassDescriptor) { // render screen
    dispatch_semaphore_wait(self.mMetalVideoCmdQueue.cmdBufferProcessSemaphore,
                            DISPATCH_TIME_FOREVER);
    sharedcmdBuffer.label = @"MetalDeviceFilter command buffer";
    __block dispatch_semaphore_t dispatchSemaphore =
        self.mMetalVideoCmdQueue.cmdBufferProcessSemaphore;
    [sharedcmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
      dispatch_semaphore_signal(dispatchSemaphore);
    }];
    id<MTLRenderCommandEncoder> renderEncoder = [sharedcmdBuffer
        renderCommandEncoderWithDescriptor:renderPassDescriptor];
    if (!renderEncoder) {
      AV_LOGE_ID(LOG_TAG, self.sessionID,
                 "[%s:%d] renderCommandEncoderWithDescriptor error .\n",
                 __FUNCTION__, __LINE__);
      return;
    }
    if ([self renderWithEncoder:renderEncoder
                           view:(RedRenderMetalView *)view]) {
      [sharedcmdBuffer presentDrawable:[view getCurrentDrawable]];
    } else {
      [renderEncoder endEncoding];
      return;
    }

    [sharedcmdBuffer commit];
    { // release
      if (self.sampleCount > 1) {
        self.msaaTexture = nil;
      }
      renderPassDescriptor.colorAttachments[0].texture = nil;
      if (self.depthPixelFormat != MTLPixelFormatInvalid) {
        renderPassDescriptor.depthAttachment.texture = nil;
        self.depthTexture = nil;
      }
      if (self.stencilPixelFormat != MTLPixelFormatInvalid) {
        renderPassDescriptor.stencilAttachment.texture = nil;
        self.stencilTexture = nil;
      }
      [view releaseDrawables];
      renderPassDescriptor = nil;
    }
  } else {
    AV_LOGE_ID(LOG_TAG, self.sessionID, "[%s:%d] error .\n", __FUNCTION__,
               __LINE__);
  }

  [super onRender:view];
}

- (void)onRenderDrawInMTKView:(RedRenderMetalView *)view {
  if (renderPipelineState) { // render screen
    dispatch_semaphore_wait(self.mMetalVideoCmdQueue.cmdBufferProcessSemaphore,
                            DISPATCH_TIME_FOREVER);
    id<MTLCommandBuffer> commandBuffer =
        [self.mMetalVideoCmdQueue.commandQueue commandBuffer];
    if (!commandBuffer) {
      AV_LOGE_ID(LOG_TAG, self.sessionID,
                 "[%s:%d] commandBuffer==nil error .\n", __FUNCTION__,
                 __LINE__);
      return;
    }
    commandBuffer.label = @"MetalDeviceFilter command buffer";
    __block dispatch_semaphore_t dispatchSemaphore =
        self.mMetalVideoCmdQueue.cmdBufferProcessSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
      dispatch_semaphore_signal(dispatchSemaphore);
    }];
    MTLRenderPassDescriptor *renderPassDescriptor1 =
        view.currentRenderPassDescriptor;
    if (renderPassDescriptor1 != nil) {
      id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer
          renderCommandEncoderWithDescriptor:renderPassDescriptor1];
      if (!renderEncoder) {
        AV_LOGE_ID(LOG_TAG, self.sessionID,
                   "[%s:%d] renderCommandEncoderWithDescriptor error .\n",
                   __FUNCTION__, __LINE__);
        return;
      }
      if ([self renderWithEncoder:renderEncoder
                             view:(RedRenderMetalView *)view]) {
        [commandBuffer presentDrawable:view.currentDrawable];
      } else {
        [renderEncoder endEncoding];
        return;
      }
    }
    [commandBuffer commit];
  } else {
    AV_LOGE_ID(LOG_TAG, self.sessionID, "[%s:%d] error .\n", __FUNCTION__,
               __LINE__);
  }

  [super onRender:view];
}

- (BOOL)renderWithEncoder:(id<MTLRenderCommandEncoder>)renderEncoder
                     view:(RedRenderMetalView *)view {
  if (!renderPipelineState || !renderEncoder) {
    AV_LOGE_ID(LOG_TAG, self.sessionID, "[%s:%d] error .\n", __FUNCTION__,
               __LINE__);
    return NO;
  }
  [renderEncoder pushDebugGroup:@"MetalDeviceFilter Render Encoder"];
  if ((self.depthPixelFormat != MTLPixelFormatInvalid ||
       self.stencilPixelFormat != MTLPixelFormatInvalid) &&
      depthStencilState) {
    [renderEncoder setDepthStencilState:depthStencilState];
  }
  [renderEncoder setRenderPipelineState:renderPipelineState];

  [self initColorConversionMatrix];
  [renderEncoder setFragmentBytes:&_colorConversionMatrix
                           length:sizeof(_colorConversionMatrix)
                          atIndex:0];

  RedRender::RotationMode rotation = RedRender::NoRotation;
  for (MetalVideoTexture *inputTexture in inputTextures) {
    [renderEncoder setFragmentTexture:inputTexture.texture
                              atIndex:inputTexture.inputTextureIndex];
    rotation = inputTexture.rotationMode;
  }
  [super initTexureCoordinateBuffer:rotation];
  [super initVerticsCoordinateBuffer];
  [renderEncoder setVertexBuffer:self.verticsCoordinateBuffer
                          offset:0
                         atIndex:0];
  [renderEncoder setVertexBuffer:self.textureCoordinateBuffer
                          offset:0
                         atIndex:1];
  [renderEncoder setVertexBytes:&modelViewProjectionMatrix
                         length:sizeof(modelViewProjectionMatrix)
                        atIndex:2];
  [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:6
                  instanceCount:1];
  [renderEncoder endEncoding];
  [renderEncoder popDebugGroup];

  return YES;
}

- (void)initColorConversionMatrix {
  switch (self.inputFrameMetaData->color_space) {
  case RedRender::VR_AVCOL_SPC_RGB:
    break;
  case RedRender::VR_AVCOL_SPC_BT709:
  case RedRender::VR_AVCOL_SPC_SMPTE240M: {
    if (self.inputFrameMetaData->color_range ==
        RedRender::VR_AVCOL_RANGE_JPEG) {
      _colorConversionMatrix = getBt709FullRangeYUV2RGBMatrix();
    } else {
      _colorConversionMatrix = getBt709VideoRangeYUV2RGBMatrix();
    }
    break;
  }
  case RedRender::VR_AVCOL_SPC_BT470BG:
  case RedRender::VR_AVCOL_SPC_SMPTE170M: {
    if (self.inputFrameMetaData->color_range ==
        RedRender::VR_AVCOL_RANGE_JPEG) {
      _colorConversionMatrix = getBt601FullRangeYUV2RGBMatrix();
    } else {
      _colorConversionMatrix = getBt601VideoRangeYUV2RGBMatrix();
    }
  } break;
  case RedRender::VR_AVCOL_SPC_BT2020_NCL:
  case RedRender::VR_AVCOL_SPC_BT2020_CL: {
    if (self.inputFrameMetaData->color_range ==
        RedRender::VR_AVCOL_RANGE_JPEG) {
      _colorConversionMatrix = getBt2020FullRangeYUV2RGBMatrix();
    } else {
      _colorConversionMatrix = getBt2020VideoRangeYUV2RGBMatrix();
    }
  } break;
  default: {
    if (self.inputFrameMetaData->pixel_format ==
            RedRender::VRPixelFormatYUV420p ||
        self.inputFrameMetaData->pixel_format ==
            RedRender::VRPixelFormatYUV420sp ||
        self.inputFrameMetaData->pixel_format ==
            RedRender::VRPixelFormatYUV420sp_vtb ||
        self.inputFrameMetaData->pixel_format ==
            RedRender::VRPixelFormatYUV420p10le) {
      if (self.inputFrameMetaData->color_range ==
          RedRender::VR_AVCOL_RANGE_JPEG) { // In case color_space=unknown
                                            // playback error
        _colorConversionMatrix = getBt709FullRangeYUV2RGBMatrix();
      } else {
        _colorConversionMatrix = getBt709VideoRangeYUV2RGBMatrix();
      }
    }
  } break;
  }
}

@end
#endif
