#pragma once

#import <UIKit/UIKit.h>

@protocol RedRenderViewProtocol <NSObject>

@property(nonatomic) BOOL doNotToggleGlviewPause;
@property(nonatomic) BOOL renderPaused;

- (UIImage *)snapshot;
- (void)shutdown;
- (void)setSmpte432ColorPrimaries;

@end
