#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../common/redrender_buffer.h"
#import "../ios/redrender_view_protocol.h"
#include "../video_inc_internal.h"
#include "../video_renderer.h"

#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface RedRenderAVSBDLView : UIImageView <RedRenderViewProtocol>

@property(readwrite, nonatomic) int sessionID;

- (instancetype)initWithFrame:(CGRect)frame sessionID:(int)sessionID;
- (void)onRender:(CVPixelBufferRef)pixelBuffer;
- (void)setLayerSessionID:(int)sessionID;
- (UIImage *)snapshot;

@end
#endif
