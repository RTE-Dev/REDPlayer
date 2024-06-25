#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../video_inc_internal.h"

#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@interface RedRenderAVSBDLLayer : AVSampleBufferDisplayLayer

@property(readwrite, nonatomic) int sessionID;

- (void)onRender:(CVPixelBufferRef)pixelBuffer;

@end

#endif
