/*
 * redrender_avsbdl_layer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "video_inc_internal.h"
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface RedRenderAVSBDLLayer : AVSampleBufferDisplayLayer

@property(readwrite, nonatomic) int sessionID;

- (void)onRender:(CVPixelBufferRef)pixelBuffer;

@end

#endif
