/*
 * redrender_metal_view.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "redrender_metal_view.h"
#import "filter/metal_filter_device.h"
#import "metal_video_renderer.h"
#import "pthread.h"
#import <os/lock.h>

typedef NS_ENUM(NSInteger, RedRenderMetalViewApplicationState) {
  RedRenderMetalViewApplicationUnknownState = 0,
  RedRenderMetalViewApplicationForegroundState = 1,
  RedRenderMetalViewApplicationBackgroundState = 2
};

@interface RedRenderMetalView ()

@end

@implementation RedRenderMetalView {
  RedRender::MetalVideoRenderer *_renderer;
  RedRender::GravityResizeAspectRatio _rendererGravity;
  MetalFilterDevice *_deviceFilter;

  RedRenderMetalViewApplicationState _applicationState;
  CAMetalLayer *_metalLayer;
  id<CAMetalDrawable> _currentDrawable;
  CGFloat _scaleFactor;

  BOOL _isLayouted;
  os_unfair_lock _lock;
}

@synthesize doNotToggleGlviewPause = _doNotToggleGlviewPause;
@synthesize renderPaused = _renderPaused;
+ (Class)layerClass {
  return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
                    sessionID:(int)sessionID
                     renderer:(void *)renderer {
  if (!(self = [super initWithFrame:frame])) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] initWithFrame error.\n",
               __FUNCTION__, __LINE__);
    return nil;
  }
  if (!renderer) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] _renderer==null error.\n",
               __FUNCTION__, __LINE__);
    return nil;
  }
  [self _init];
  [self updateLayerDrawableSize:frame.size];
  _renderer = (RedRender::MetalVideoRenderer *)renderer;
  _sessionID = sessionID;
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return self;
}

- (id<CAMetalDrawable>)getCurrentDrawable {
  if (_currentDrawable == nil && _metalLayer != nil) {
    _currentDrawable = [_metalLayer nextDrawable];
  }
  return _currentDrawable;
}

- (void)releaseDrawables {
  _currentDrawable = nil;
}

- (void)_init {
  _mMetalVideoCmdQueue = [MetalVideoCmdQueue getCmdQueueInstance];
  if (!_mMetalVideoCmdQueue) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] getCmdQueueInstance = nil error .\n", __FUNCTION__,
               __LINE__);
    return;
  }
  _metalLayer = (CAMetalLayer *)self.layer;
  _metalLayer.device = _mMetalVideoCmdQueue.device;
  _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  _metalLayer.framebufferOnly = YES;
  self.paused = YES;
  self.autoResizeDrawable = YES;
  self.enableSetNeedsDisplay = NO;

  self.device = _mMetalVideoCmdQueue.device;
  self.layer.opaque = NO;

  _metalLayer.opaque = NO;
  [self registerApplicationObservers];
  [self setup];
  self.delegate = self;
  _applicationState = RedRenderMetalViewApplicationForegroundState;
}

- (RedRender::VRAVColorSpace)getColorSpace:(CVPixelBufferRef)pixelBuffer {
  if (pixelBuffer == nil) {
    return RedRender::VR_AVCOL_SPC_NB;
  }

  NSString *colorAttachments = (__bridge NSString *)CVBufferGetAttachment(
      pixelBuffer, kCVImageBufferYCbCrMatrixKey, NULL);
  if (colorAttachments == nil) {
    return RedRender::VR_AVCOL_SPC_BT709;
  } else if (colorAttachments != nil &&
             CFStringCompare((__bridge CFStringRef)colorAttachments,
                             kCVImageBufferYCbCrMatrix_ITU_R_709_2,
                             0) == kCFCompareEqualTo) {
    return RedRender::VR_AVCOL_SPC_BT709;
  } else if (colorAttachments != nil &&
             CFStringCompare((__bridge CFStringRef)colorAttachments,
                             kCVImageBufferYCbCrMatrix_ITU_R_601_4,
                             0) == kCFCompareEqualTo) {
    return RedRender::VR_AVCOL_SPC_BT470BG;
  } else {
    return RedRender::VR_AVCOL_SPC_BT709;
  }

  return RedRender::VR_AVCOL_SPC_NB;
}

- (void)dealloc {
  AV_LOGI_ID(LOG_TAG, _sessionID, "[%s:%d] start .\n", __FUNCTION__, __LINE__);
  _renderer = nil;
  _deviceFilter = nil;
  _mMetalVideoCmdQueue = nil;
  _currentDrawable = nil;
  _metalLayer = nil;
  AV_LOGI_ID(LOG_TAG, _sessionID, "[%s:%d] end .\n", __FUNCTION__, __LINE__);
}

- (void)setSmpte432ColorPrimaries {
  if ([self.layer isKindOfClass:[CAMetalLayer class]]) {
    CGColorSpaceRef colorSpace =
        CGColorSpaceCreateWithName(kCGColorSpaceDisplayP3);
    ((CAMetalLayer *)self.layer).colorspace = colorSpace;
    CGColorSpaceRelease(colorSpace);
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] RedRenderMetalView display: CAMetalLayer support "
               "AVCOL_PRI_SMPTE432 .\n",
               __FUNCTION__, __LINE__);
  } else {
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] RedRenderMetalView display: CAMetalLayer cannot "
               "support AVCOL_PRI_SMPTE432 .\n",
               __FUNCTION__, __LINE__);
  }
}

#pragma mark - Private Method
- (void)setup {
  _scaleFactor = [[UIScreen mainScreen] scale];
  if (_scaleFactor < 0.1f) {
    _scaleFactor = 1.0f;
  }
  [self setScaleFactor:_scaleFactor];
}

- (CGSize)updateLayerDrawableSize:(CGSize)size {
  if (_layerSizeUpdate) {
    CGSize drawableSize = self.bounds.size;
    if (size.width > 0 && size.height > 0) {
      drawableSize = size;
    } else {
      drawableSize.width *= self.contentScaleFactor;
      drawableSize.height *= self.contentScaleFactor;
    }

    if (drawableSize.width == 0 || drawableSize.height == 0) {
      drawableSize.width = 1;
      drawableSize.height = 1;
    }
    _metalLayer.drawableSize = drawableSize;
    _layerSizeUpdate = NO;
  }
  _mDrawableSize = _metalLayer.drawableSize;
  return _metalLayer.drawableSize;
}

#pragma mark - Override

- (void)setDoNotToggleGlviewPause:(BOOL)doNotToggleGlviewPause {
  _doNotToggleGlviewPause = doNotToggleGlviewPause;
}

- (void)setScaleFactor:(CGFloat)scaleFactor {
  _scaleFactor = scaleFactor;
  hanleInMainThread(^{
    [self->_metalLayer setContentsScale:self->_scaleFactor];
  });
  [self invalidateRenderBuffer];
}

- (void)didMoveToWindow {
  self.contentScaleFactor = [UIScreen mainScreen].nativeScale;
}

- (void)setContentScaleFactor:(CGFloat)contentScaleFactor {
  [super setContentScaleFactor:contentScaleFactor];
  _layerSizeUpdate = YES;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _layerSizeUpdate = YES;
  [self updateLayoutStatus];
  if (self.window.screen != nil) {
    _scaleFactor = self.window.screen.scale;
    [self.layer setContentsScale:_scaleFactor];
  }
  [self invalidateRenderBuffer];
}

- (void)setFrame:(CGRect)frame {
  [super setFrame:frame];
  [self updateLayoutStatus];
}

- (void)setContentMode:(UIViewContentMode)contentMode {
  [super setContentMode:contentMode];
  switch (contentMode) {
  case UIViewContentModeScaleToFill:
    _rendererGravity = RedRender::AspectRatioFill;
    break;
  case UIViewContentModeScaleAspectFit:
    _rendererGravity = RedRender::ScaleAspectFit;
    break;
  case UIViewContentModeScaleAspectFill:
    _rendererGravity = RedRender::ScaleAspectFill;
    break;
  default:
    _rendererGravity = RedRender::ScaleAspectFit;
    break;
  }
  os_unfair_lock_lock(&_lock);
  if (_renderer) {
    _renderer->setGravity(_rendererGravity);
  }
  os_unfair_lock_unlock(&_lock);
  [self invalidateRenderBuffer];
}

static void hanleInMainThread(dispatch_block_t mainThreadblock) {
  if (0 != pthread_main_np()) {
    mainThreadblock();
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      mainThreadblock();
    });
  }
}

- (BOOL)isApplicationActive {
  switch (_applicationState) {
  case RedRenderMetalViewApplicationForegroundState:
    return YES;
  case RedRenderMetalViewApplicationBackgroundState:
    return NO;
  default: {
    __block UIApplicationState appState = UIApplicationStateActive;
    hanleInMainThread(^{
      appState = [UIApplication sharedApplication].applicationState;
    });
    switch (appState) {
    case UIApplicationStateActive:
      return YES;
    case UIApplicationStateInactive:
    case UIApplicationStateBackground:
    default:
      return NO;
    }
  }
  }
}

- (void)updateLayoutStatus {
  _isLayouted = !(self.bounds.size.width == 0 || self.bounds.size.height == 0);
}

- (bool)isDisplay {
  if ([self isApplicationActive] == NO && _doNotToggleGlviewPause == NO) {
    return NO;
  }
  if (!_isLayouted) {
    return NO;
  }
  return YES;
}

- (void)invalidateRenderBuffer {
  if (!self.renderPaused)
    return;
  if (0 != pthread_main_np()) {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
      os_unfair_lock_lock(&self->_lock);
      if (self->_renderer) {
        self->_renderer->onRenderCachedFrame();
      }
      os_unfair_lock_unlock(&self->_lock);
    });
  }
}

- (void)setRenderer:(void *)renderer {
  os_unfair_lock_lock(&_lock);
  _renderer = (RedRender::MetalVideoRenderer *)renderer;
  os_unfair_lock_unlock(&_lock);
}

- (void)setMetalFilterDevice:(void *)deviceFilter {
  _deviceFilter = (__bridge MetalFilterDevice *)deviceFilter;
}

#pragma mark - Delegate

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
  _layerSizeUpdate = YES;
  [self updateLayerDrawableSize:(CGSize)size];
}

- (void)drawInMTKView:(nonnull MTKView *)view {
  void (^block)(void) = ^() {
    @autoreleasepool {
      if (self->_deviceFilter) {
        [self->_deviceFilter onRenderDrawInMTKView:self];
      } else {
        AV_LOGE_ID(LOG_TAG, self->_sessionID,
                   "[%s:%d] _deviceFilter==nil error .\n", __FUNCTION__,
                   __LINE__);
      }
    }
  };
  block();
}

#pragma mark AppDelegate
- (void)registerApplicationObservers {
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationWillEnterForeground)
             name:UIApplicationWillEnterForegroundNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationDidBecomeActive)
             name:UIApplicationDidBecomeActiveNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationWillResignActive)
             name:UIApplicationWillResignActiveNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationDidEnterBackground)
             name:UIApplicationDidEnterBackgroundNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationWillTerminate)
             name:UIApplicationWillTerminateNotification
           object:nil];
}

- (void)applicationWillEnterForeground {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderMetalView applicationWillEnterForeground:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderMetalViewApplicationForegroundState;
}

- (void)applicationDidBecomeActive {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderMetalView applicationDidBecomeActive:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderMetalViewApplicationForegroundState;
}

- (void)applicationWillResignActive {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderMetalView applicationWillResignActive:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderMetalViewApplicationBackgroundState;
}

- (void)applicationDidEnterBackground {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderMetalView applicationDidEnterBackground:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderMetalViewApplicationBackgroundState;
}

- (void)applicationWillTerminate {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderMetalView applicationWillTerminate:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
}

- (void)shutdown {
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
