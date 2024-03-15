/*
 * redrender_gl_view.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef REDRENDER_GL_VIEW_H
#define REDRENDER_GL_VIEW_H

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
#endif /* REDRENDER_GL_VIEW_H */
