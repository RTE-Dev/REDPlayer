/*
 * redrender_avsbdl_view.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "redrender_avsbdl_view.h"
#import "redrender_avsbdl_layer.h"
#import <AVFoundation/AVSampleBufferDisplayLayer.h>
#import <pthread/pthread.h>

@implementation RedRenderAVSBDLView {
  RedRenderAVSBDLLayer *_sampleBufferDisplayLayer;
  CGFloat _scaleFactor;
}

@synthesize doNotToggleGlviewPause = _doNotToggleGlviewPause;
@synthesize renderPaused = _renderPaused;
- (instancetype)initWithFrame:(CGRect)frame sessionID:(int)sessionID {
  if (!(self == [super initWithFrame:frame])) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] init failed!\n", __FUNCTION__,
               __LINE__);
    return nil;
  }
  _sessionID = sessionID;
  _sampleBufferDisplayLayer = [[RedRenderAVSBDLLayer alloc] init];
  _sampleBufferDisplayLayer.sessionID = sessionID;
  [self.layer addSublayer:_sampleBufferDisplayLayer];

  _scaleFactor = [[UIScreen mainScreen] scale];
  if (_scaleFactor < 0.1f) {
    _scaleFactor = 1.0f;
  }
  [self setScaleFactor:_scaleFactor];

  return self;
}

- (void)setScaleFactor:(CGFloat)scaleFactor {
  _scaleFactor = scaleFactor;
  [_sampleBufferDisplayLayer setContentsScale:_scaleFactor];
}

- (void)dealloc {
  _sampleBufferDisplayLayer = nil;
}

- (void)onRender:(CVPixelBufferRef)pixelBuffer {
  void (^block)(void) = ^{
    [self->_sampleBufferDisplayLayer onRender:pixelBuffer];
  };

  if (0 != pthread_main_np()) {
    block();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      block();
    });
  }
}

- (void)setLayerSessionID:(int)sessionID {
  _sampleBufferDisplayLayer.sessionID = sessionID;
}

- (void)setSmpte432ColorPrimaries {
}

- (void)shutdown {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Override
- (void)layoutSubviews {
  [super layoutSubviews];

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  if (self.window.screen != nil) {
    _scaleFactor = self.window.screen.scale;
    [_sampleBufferDisplayLayer setContentsScale:_scaleFactor];
  }
  _sampleBufferDisplayLayer.frame = self.bounds;
  _sampleBufferDisplayLayer.position =
      CGPointMake(CGRectGetMidX(self.bounds), CGRectGetMidY(self.bounds));
  [CATransaction commit];
}

- (void)setContentMode:(UIViewContentMode)contentMode {
  [super setContentMode:contentMode];
  switch (contentMode) {
  case UIViewContentModeScaleToFill:
    _sampleBufferDisplayLayer.videoGravity = AVLayerVideoGravityResize;
    break;
  case UIViewContentModeScaleAspectFit:
    _sampleBufferDisplayLayer.videoGravity = AVLayerVideoGravityResizeAspect;
    break;
  case UIViewContentModeScaleAspectFill:
    _sampleBufferDisplayLayer.videoGravity =
        AVLayerVideoGravityResizeAspectFill;
    break;
  default:
    break;
  }
}

#pragma mark snapshot
- (UIImage *)snapshot {
  UIImage *image = [self snapshotInternal];
  return image;
}

- (UIImage *)snapshotInternal {
  return [self snapshotInternalOnIOS7AndLater];
}

- (UIImage *)snapshotInternalOnIOS7AndLater {
  if (self.bounds.size.width < FLT_EPSILON ||
      self.bounds.size.height < FLT_EPSILON) {
    return nil;
  }

  UIGraphicsBeginImageContextWithOptions(self.bounds.size, NO, 0.0);
  // Render our snapshot into the image context
  [self drawViewHierarchyInRect:self.bounds afterScreenUpdates:NO];

  // Grab the image from the context
  UIImage *complexViewImage = UIGraphicsGetImageFromCurrentImageContext();
  // Finish using the context
  UIGraphicsEndImageContext();

  return complexViewImage;
}

@end
#endif
