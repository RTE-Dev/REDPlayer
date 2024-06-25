#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../../common/redrender_buffer.h"
#import "metal_common.h"
#import "metal_video_cmd_queue.h"
#import "metal_video_output.h"
#import "metal_video_texture.h"

#include "../../video_inc_internal.h"
#import "redrender_metal_view.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

typedef struct FilterPipelineState {
  __weak NSString *filterNameStr;
  RedRender::VideoFilterType metalFilterType;
  __weak NSString *fragmentFuncNameStr;
  __weak NSString *vertexFuncNameStr;
  __weak NSString *computeFuncNameStr;
  MTLPixelFormat depthPixelFormat;
  MTLPixelFormat stencilPixelFormat;
  int inputTexNum;
  int sampleCount;
  BOOL depthWriteEnabled;
  int pixelFormat;
} FilterPipelineState;

@interface MetalFilterBase : MetalVideoOutput <MetalVideoInput> {
  MetalVideoCmdQueue *filterCmdQueue;

  __weak NSString *fragmentFuncNameStr;
  __weak NSString *vertexFuncNameStr;
  __weak NSString *computeFuncNameStr;
  NSPointerArray *inputTextures;
  NSMutableArray *inputTextureIndexs;
  NSMutableArray *inputTexturesRotationMode;

  id<MTLComputePipelineState> computePipelineState;
  id<MTLRenderPipelineState> renderPipelineState;
  MTLRenderPipelineDescriptor *renderPipelineStateDescriptor;
  MTLRenderPassDescriptor *renderPassDescriptor;
  id<MTLDepthStencilState> depthStencilState;
  MTLDepthStencilDescriptor *renderDepthStateDesc;
  MTLSize threadsPerGrid;
  MTLSize threadgroupsPerGrid;
  MTLSize threadsPerThreadgroup;

  float verticesCoordinate[8];
  float texureCoordinate[8];
  matrix_float4x4 modelViewProjectionMatrix;
}

@property(readonly, nonatomic) MetalVideoCmdQueue *mMetalVideoCmdQueue;
@property(readonly, nonatomic) id<MTLDevice> filterDevice;
@property(readwrite, nonatomic) id<MTLLibrary> filterLibrary;

@property(readwrite, nonatomic) int inputTexNum;
@property(readwrite, nonatomic) int sampleCount;
@property(readwrite, nonatomic) BOOL depthWriteEnabled;
@property(readwrite, nonatomic) id<MTLBuffer> verticsCoordinateBuffer;
@property(readwrite, nonatomic) id<MTLBuffer> textureCoordinateBuffer;
@property(nonatomic) MTLPixelFormat depthPixelFormat;
@property(nonatomic) MTLPixelFormat stencilPixelFormat;
@property(readwrite, nonatomic) id<MTLTexture> depthTexture;
@property(readwrite, nonatomic) id<MTLTexture> stencilTexture;
@property(readwrite, nonatomic) id<MTLTexture> msaaTexture;
@property(readwrite, nonatomic) int sessionID;
@property(readwrite, nonatomic) int pixelFormat;

@property(readonly, nonatomic) __weak NSString *filterNameStr;
@property(readonly, nonatomic) RedRender::VideoFilterType metalFilterType;
@property(readwrite, nonatomic)
    RedRender::VideoFrameMetaData *inputFrameMetaData;
@property(readwrite, nonatomic)
    int paddingPixels; // Used to crop redundant textures

@property(strong, nonatomic) id<CAMetalDrawable> filterDrawable;

- (id)initWithFilterPipelineState:(MetalVideoCmdQueue *)metalVideoCmdQueue
              filterPipelineState:(FilterPipelineState *)filterPipelineState
                        sessionID:(int)sessionID;
- (void)onRender:(RedRenderMetalView *)view;
- (BOOL)renderWithEncoder:(id<MTLRenderCommandEncoder>)renderEncoder;
- (BOOL)updateParam:(RedRenderMetalView *)view;
- (void)initVerticsCoordinateBuffer;
- (void)initTexureCoordinateBuffer:(RedRender::RotationMode)rotationMode;

@end
#endif
