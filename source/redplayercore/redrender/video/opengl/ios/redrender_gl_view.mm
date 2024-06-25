/*
 * redrender_gl_view.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "redrender_gl_view.h"

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "opengl_video_renderer.h"
#import "pthread.h"
#import <CoreFoundation/CFString.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/glext.h>

typedef NS_ENUM(NSInteger, RedRenderOpenGLViewApplicationState) {
  RedRenderOpenGLViewApplicationUnknownState = 0,
  RedRenderOpenGLViewApplicationForegroundState = 1,
  RedRenderOpenGLViewApplicationBackgroundState = 2
};

@interface RedRenderGLView () {
  EAGLContext *_prevContext;
  GLuint _framebuffer;
  GLuint _renderbuffer;
  GLint _backgroundWidth;
  GLint _backgroundHeight;

  NSRecursiveLock *_glActiveLock;
  int _sessionID;
  bool _isRenderBufferInvalidated;
  BOOL _didSetupGL;

  NSMutableArray *_registeredNotifications;
  RedRenderOpenGLViewApplicationState _applicationState;

  CGFloat _scaleFactor;
  BOOL _isLayouted;

  RedRender::OpenGLVideoRenderer *_renderer;
  RedRender::GravityResizeAspectRatio _rendererGravity;
}
@end

@implementation RedRenderGLView

@synthesize doNotToggleGlviewPause = _doNotToggleGlviewPause;
@synthesize renderPaused = _renderPaused;
+ (Class)layerClass {
  return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
                    sessionID:(int)sessionID
                     renderer:(void *)renderer {
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  if (!(self = [super initWithFrame:frame])) {
    return nil;
  }

  if (!_renderer) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] _renderer==null error .\n",
               __FUNCTION__, __LINE__);
  }

  if (![self _init]) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] init error .\n", __FUNCTION__,
               __LINE__);
    return nil;
  }
  _sessionID = sessionID;
  _renderer = (RedRender::OpenGLVideoRenderer *)renderer;
  _applicationState = RedRenderOpenGLViewApplicationForegroundState;

  return self;
}

- (CAEAGLLayer *)eaglLayer {
  return (CAEAGLLayer *)self.layer;
}

- (BOOL)_init {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _glActiveLock = [[NSRecursiveLock alloc] init];

  CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
  eaglLayer.opaque = YES;
  eaglLayer.drawableProperties = [NSDictionary
      dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:NO],
                                   kEAGLDrawablePropertyRetainedBacking,
                                   kEAGLColorFormatRGBA8,
                                   kEAGLDrawablePropertyColorFormat, nil];
  CGFloat _scaleFactor = [[UIScreen mainScreen] scale];
  if (_scaleFactor < 0.1f) {
    _scaleFactor = 1.0f;
  }
  [eaglLayer setContentsScale:_scaleFactor];

  _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  if (_context == nil) {
    AV_LOGE_ID(
        LOG_TAG, _sessionID,
        "[%s:%d] failed to setup kEAGLRenderingAPIOpenGLES3 error=0x%04X .\n",
        __FUNCTION__, __LINE__, glGetError());
    _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (_context == nil) {
      AV_LOGE_ID(
          LOG_TAG, _sessionID,
          "[%s:%d] failed to setup kEAGLRenderingAPIOpenGLES2 error=0x%04X .\n",
          __FUNCTION__, __LINE__, glGetError());
      return NO;
    } else {
      AV_LOGI_ID(LOG_TAG, _sessionID,
                 "[%s:%d] init context kEAGLRenderingAPIOpenGLES2 success .\n",
                 __FUNCTION__, __LINE__);
    }
  } else {
    AV_LOGI_ID(LOG_TAG, _sessionID,
               "[%s:%d] init context kEAGLRenderingAPIOpenGLES3 success .\n",
               __FUNCTION__, __LINE__);
  }
  EAGLContext *prevContext = [EAGLContext currentContext];
  [EAGLContext setCurrentContext:_context];
  _didSetupGL = NO;
  if ([self _setupEAGLContext:_context]) {
    AV_LOGI_ID(LOG_TAG, _sessionID, "[%s:%d] _setupEAGLContext success .\n",
               __FUNCTION__, __LINE__);
    _didSetupGL = YES;
  }
  [EAGLContext setCurrentContext:prevContext];

  return YES;
}

- (void)dealloc {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _renderer = nil;
  EAGLContext *prevContext = [EAGLContext currentContext];
  [EAGLContext setCurrentContext:_context];
  if (_framebuffer) {
    glDeleteFramebuffers(1, &_framebuffer);
    _framebuffer = 0;
  }

  if (_renderbuffer) {
    glDeleteRenderbuffers(1, &_renderbuffer);
    _renderbuffer = 0;
  }

  if (_textureCache) {
    CVOpenGLESTextureCacheFlush(_textureCache, 0);
    CFRelease(_textureCache);
    _textureCache = nil;
  }

  glFinish();
  [EAGLContext setCurrentContext:prevContext];
  _context = nil;
  _glActiveLock = nil;

  [self unregisterApplicationObservers];
}

- (void)useAsCurrent {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  _prevContext = [EAGLContext currentContext];
  [EAGLContext setCurrentContext:_context];
}

- (void)useAsPrev {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  [EAGLContext setCurrentContext:_prevContext];
}

- (void)presentBufferForDisplay {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
  [_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)activeFramebuffer {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  EAGLContext *prevContext = [EAGLContext currentContext];
  [EAGLContext setCurrentContext:_context];
  if (_isRenderBufferInvalidated) {
    _isRenderBufferInvalidated = false;
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    hanleInMainThread(^{
      [self->_context renderbufferStorage:GL_RENDERBUFFER
                             fromDrawable:(CAEAGLLayer *)self.layer];
    });
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,
                                 &_backgroundWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT,
                                 &_backgroundHeight);
    if (_renderer != nullptr) {
      _renderer->setGravity(_rendererGravity);
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  glViewport(0, 0, _backgroundWidth, _backgroundHeight);
  [EAGLContext setCurrentContext:prevContext];
}

- (void)inActiveFramebuffer {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

- (int)getFrameWidth {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _backgroundWidth;
}

- (int)getFrameHeigh {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  return _backgroundHeight;
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

- (BOOL)_setupEAGLContext:(EAGLContext *)context {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  glGenFramebuffers(1, &_framebuffer);
  glGenRenderbuffers(1, &_renderbuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
  [context renderbufferStorage:GL_RENDERBUFFER
                  fromDrawable:(CAEAGLLayer *)self.layer];
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,
                               &_backgroundWidth);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT,
                               &_backgroundHeight);
  CHECK_OPENGL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                         GL_RENDERBUFFER, _renderbuffer));

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] glCheckFramebufferStatus error : %x.\n", __FUNCTION__,
               __LINE__, status);
    return NO;
  }

  CVReturn ret = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL,
                                              context, NULL, &_textureCache);
  if (ret || _textureCache == nil) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] CVOpenGLESTextureCacheCreate error:%d .\n",
               __FUNCTION__, __LINE__, (int32_t)ret);
  }

  GLenum glError = glGetError();
  if (GL_NO_ERROR != glError) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] failed to setup GL error=0x%04X.\n", __FUNCTION__,
               __LINE__, glError);
    return NO;
  }

  return YES;
}

- (void)lockGLActive {
  [_glActiveLock lock];
}

- (void)unlockGLActive {
  [_glActiveLock unlock];
}

- (BOOL)setupGLOnce {
  if (_didSetupGL)
    return YES;

  BOOL didSetupGL = [self _init];
  return didSetupGL;
}

- (void)setScaleFactor:(CGFloat)scaleFactor {
  _scaleFactor = scaleFactor;
  hanleInMainThread(^{
    [[self eaglLayer] setContentsScale:self->_scaleFactor];
  });
  [self invalidateRenderBuffer];
}

- (void)setDoNotToggleGlviewPause:(BOOL)doNotToggleGlviewPause {
  _doNotToggleGlviewPause = doNotToggleGlviewPause;
}

- (void)shutdown {
}

- (void)setSmpte432ColorPrimaries {
}

- (void)layoutSubviews {
  [super layoutSubviews];
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
  _renderer->setGravity(_rendererGravity);
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
  case RedRenderOpenGLViewApplicationForegroundState:
    return YES;
  case RedRenderOpenGLViewApplicationBackgroundState:
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
  [self lockGLActive];
  if (0 != pthread_main_np()) {
    EAGLContext *prevContext = [EAGLContext currentContext];
    [EAGLContext setCurrentContext:_context];
    [EAGLContext setCurrentContext:prevContext];
  }
  _isRenderBufferInvalidated = true;
  [self unlockGLActive];
}

- (void)registerApplicationObservers {

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationWillEnterForeground)
             name:UIApplicationWillEnterForegroundNotification
           object:nil];
  [_registeredNotifications
      addObject:UIApplicationWillEnterForegroundNotification];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationDidBecomeActive)
             name:UIApplicationDidBecomeActiveNotification
           object:nil];
  [_registeredNotifications addObject:UIApplicationDidBecomeActiveNotification];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationWillResignActive)
             name:UIApplicationWillResignActiveNotification
           object:nil];
  [_registeredNotifications
      addObject:UIApplicationWillResignActiveNotification];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationDidEnterBackground)
             name:UIApplicationDidEnterBackgroundNotification
           object:nil];
  [_registeredNotifications
      addObject:UIApplicationDidEnterBackgroundNotification];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(applicationWillTerminate)
             name:UIApplicationWillTerminateNotification
           object:nil];
  [_registeredNotifications addObject:UIApplicationWillTerminateNotification];
}

- (void)unregisterApplicationObservers {
  for (NSString *name in _registeredNotifications) {
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:name
                                                  object:nil];
  }
}

- (void)applicationWillEnterForeground {
  [self setupGLOnce];
  _applicationState = RedRenderOpenGLViewApplicationForegroundState;
  AV_LOGI_ID(
      LOG_TAG, _sessionID,
      "[%s:%d] RedRenderOpenGLView applicationWillEnterForeground:%d .\n",
      __FUNCTION__, __LINE__,
      (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderOpenGLViewApplicationForegroundState;
}

- (void)applicationDidBecomeActive {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderOpenGLView applicationDidBecomeActive:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderOpenGLViewApplicationForegroundState;
  glFinish();
}

- (void)applicationWillResignActive {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderOpenGLView applicationWillResignActive:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderOpenGLViewApplicationBackgroundState;
  glFinish();
}

- (void)applicationDidEnterBackground {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderOpenGLView applicationDidEnterBackground:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
  _applicationState = RedRenderOpenGLViewApplicationBackgroundState;
}

- (void)applicationWillTerminate {
  AV_LOGI_ID(LOG_TAG, _sessionID,
             "[%s:%d] RedRenderOpenGLView applicationWillTerminate:%d .\n",
             __FUNCTION__, __LINE__,
             (int)[UIApplication sharedApplication].applicationState);
}

#pragma mark snapshot

- (UIImage *)snapshot {
  [self lockGLActive];
  UIImage *image = [self snapshotInternal];
  [self unlockGLActive];
  return image;
}

- (UIImage *)snapshotInternal {
  if (@available(iOS 7.0, *)) {
    return [self snapshotInternalOnIOS7AndLater];
  } else {
    return [self snapshotInternalOnIOS6AndBefore];
  }
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

- (UIImage *)snapshotInternalOnIOS6AndBefore {
  EAGLContext *prevContext = [EAGLContext currentContext];
  [EAGLContext setCurrentContext:_context];

  GLint backingWidth, backingHeight;

  // Bind the color renderbuffer used to render the OpenGL ES view
  // If your application only creates a single color renderbuffer which is
  // already bound at this point, this call is redundant, but it is needed if
  // you're dealing with multiple renderbuffers. Note, replace
  // "viewRenderbuffer" with the actual name of the renderbuffer object defined
  // in your class.
  glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);

  // Get the size of the backing CAEAGLLayer
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,
                               &backingWidth);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT,
                               &backingHeight);

  NSInteger x = 0, y = 0, width = backingWidth, height = backingHeight;
  if (width < FLT_EPSILON || height < FLT_EPSILON) {
    return nil;
  }

  NSInteger dataLength = width * height * 4;
  GLubyte *data = (GLubyte *)malloc(dataLength * sizeof(GLubyte));

  // Read pixel data from the framebuffer
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glReadPixels((int)x, (int)y, (int)width, (int)height, GL_RGBA,
               GL_UNSIGNED_BYTE, data);

  // Create a CGImage with the pixel data
  // If your OpenGL ES content is opaque, use kCGImageAlphaNoneSkipLast to
  // ignore the alpha channel otherwise, use kCGImageAlphaPremultipliedLast
  CGDataProviderRef ref =
      CGDataProviderCreateWithData(NULL, data, dataLength, NULL);
  CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
  CGImageRef iref =
      CGImageCreate(width, height, 8, 32, width * 4, colorspace,
                    kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast,
                    ref, NULL, true, kCGRenderingIntentDefault);

  [EAGLContext setCurrentContext:prevContext];

  // OpenGL ES measures data in PIXELS
  // Create a graphics context with the target size measured in POINTS
  UIGraphicsBeginImageContext(CGSizeMake(width, height));

  CGContextRef cgcontext = UIGraphicsGetCurrentContext();
  // UIKit coordinate system is upside down to GL/Quartz coordinate system
  // Flip the CGImage by rendering it to the flipped bitmap context
  // The size of the destination area is measured in POINTS
  CGContextSetBlendMode(cgcontext, kCGBlendModeCopy);
  CGContextDrawImage(cgcontext, CGRectMake(0.0, 0.0, width, height), iref);

  // Retrieve the UIImage from the current context
  UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();

  // Clean up
  free(data);
  CFRelease(ref);
  CFRelease(colorspace);
  CGImageRelease(iref);

  return image;
}

@end
#endif
