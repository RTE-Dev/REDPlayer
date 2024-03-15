/*
 * redrender_view_protocol.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef REDRENDER_VIEW_PROTOCOL_H
#define REDRENDER_VIEW_PROTOCOL_H

#import <UIKit/UIKit.h>

@protocol RedRenderViewProtocol <NSObject>

@property(nonatomic) BOOL doNotToggleGlviewPause;
@property(nonatomic) BOOL renderPaused;

/// 全局只设置一次的的实验开关
@property(nonatomic, strong) NSDictionary *justOnceExpSwitch;

/// 全局只设置一次的的配置开关
@property(nonatomic, strong) NSDictionary *justOnceConfigSwitch;

- (UIImage *)snapshot;
- (void)shutdown;
- (void)setSmpte432ColorPrimaries;

@end

#endif /* VIDEO_INC_INTERNAL_H */
