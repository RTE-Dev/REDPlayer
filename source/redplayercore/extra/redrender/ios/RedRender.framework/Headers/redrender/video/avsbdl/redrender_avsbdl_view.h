/*
 * redrender_avsbdl_view.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../common/redrender_buffer.h"
#import "../ios/redrender_view_protocol.h"
#include "../video_renderer.h"
#include "video_inc_internal.h"
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface RedRenderAVSBDLView : UIImageView <RedRenderViewProtocol>

@property(readwrite, nonatomic) int sessionID;
@property(readwrite, nonatomic) BOOL renderOptimize;

- (instancetype)initWithFrame:(CGRect)frame sessionID:(int)sessionID;
- (void)onRender:(CVPixelBufferRef)pixelBuffer;
- (void)setLayerSessionID:(int)sessionID;
- (UIImage *)snapshot;
- (RedRender::VRError)
    updateSubViewMetal:(RedRender::VideoFrameMetaData *)inputFrameMetaData
      metalVideoRender:
          (std::shared_ptr<RedRender::VideoRenderer>)metalVideoRender;

@end

#endif
