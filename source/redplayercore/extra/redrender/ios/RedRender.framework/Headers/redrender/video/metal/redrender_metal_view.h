/*
 * redrender_metal_view.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "../ios/redrender_view_protocol.h"
#include "../video_inc_internal.h"
#import "common/metal_video_cmd_queue.h"
#import "common/metal_video_texture.h"
#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>

@interface RedRenderMetalView : MTKView <MTKViewDelegate, RedRenderViewProtocol>

@property(readwrite, nonatomic) int sessionID;
@property(readwrite, nonatomic) BOOL layerSizeUpdate;
@property(readonly, nonatomic) MetalVideoCmdQueue *mMetalVideoCmdQueue;
@property(readonly, nonatomic) CGSize mDrawableSize;

- (instancetype)initWithFrame:(CGRect)frame
                    sessionID:(int)sessionID
                     renderer:(void *)renderer;
- (id<CAMetalDrawable>)getCurrentDrawable;
- (void)releaseDrawables;
- (CGSize)updateLayerDrawableSize:(CGSize)size;
- (void)setSmpte432ColorPrimaries;
- (UIImage *)snapshot;
- (bool)isDisplay;
- (RedRender::VRAVColorSpace)getColorSpace:(CVPixelBufferRef)pixelBuffer;
- (void)setRenderer:(void *)renderer;
- (void)setMetalFilterDevice:(void *)deviceFilter;

@end

#endif
