#pragma once

#include "../../video_inc_internal.h"
#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "../../ios/redrender_view_protocol.h"
#import <UIKit/UIKit.h>

@interface RedRenderGLView : UIView <RedRenderViewProtocol>

@property(readonly, nonatomic) CVOpenGLESTextureCacheRef textureCache;
@property(readonly, nonatomic) EAGLContext *context;

- (instancetype)initWithFrame:(CGRect)frame
                    sessionID:(int)sessionID
                     renderer:(void *)renderer;
- (void)useAsCurrent;
- (void)useAsPrev;
- (void)presentBufferForDisplay;
- (void)activeFramebuffer;
- (void)inActiveFramebuffer;
- (int)getFrameWidth;
- (int)getFrameHeigh;
- (RedRender::VRAVColorSpace)getColorSpace:(CVPixelBufferRef)pixelBuffer;
- (bool)isDisplay;
- (UIImage *)snapshot;
- (void)setSmpte432ColorPrimaries;

@end

#endif
