/*
 * redrender_avsbdl_layer.mm
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "redrender_avsbdl_layer.h"
#import <AVFoundation/AVSampleBufferDisplayLayer.h>
#import <pthread/pthread.h>

@implementation RedRenderAVSBDLLayer {
  BOOL _invisible;
  BOOL _renderDisabled;
  pthread_mutex_t _renderLock;
}

- (instancetype)init {
  if (!(self == [super init])) {
    AV_LOGE(LOG_TAG, "[%s:%d] init failed!\n", __FUNCTION__, __LINE__);
    return nil;
  }
  _sessionID = 0;
  self.invisible = YES;
  self.opaque = YES;
  self.videoGravity = AVLayerVideoGravityResizeAspect;
  self.backgroundColor = [UIColor clearColor].CGColor;
  pthread_mutex_init(&_renderLock, NULL);

  self.renderDisabled = [UIApplication sharedApplication].applicationState !=
                        UIApplicationStateActive;

  for (NSString *key in @[
         UIApplicationWillEnterForegroundNotification,
         UIApplicationDidBecomeActiveNotification
       ]) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(applicationEnterForeground:)
               name:key
             object:nil];
  }

  for (NSString *key in @[
         UIApplicationWillResignActiveNotification,
         UIApplicationDidEnterBackgroundNotification,
         UIApplicationWillTerminateNotification
       ]) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(applicationEnterBackground:)
               name:key
             object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  pthread_mutex_destroy(&_renderLock);
}

- (void)onRender:(CVPixelBufferRef)pixelBuffer {
  if (!pixelBuffer) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] pixelBuffer==nil error .\n",
               __FUNCTION__, __LINE__);
    return;
  }

  pthread_mutex_lock(&_renderLock);
  if (_renderDisabled) {
    pthread_mutex_unlock(&_renderLock);
    return;
  }

  CMVideoFormatDescriptionRef desc = NULL;
  CFRetain(pixelBuffer);
  CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer,
                                               &desc);
  if (desc) {
    CMSampleBufferRef sampleBuffer = NULL;
    CMSampleTimingInfo timingInfo = {kCMTimeIndefinite, kCMTimeInvalid,
                                     kCMTimeInvalid};
    CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, true,
                                       NULL, NULL, desc, &timingInfo,
                                       &sampleBuffer);
    if (sampleBuffer) {
      CFArrayRef attachments =
          CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true);
      CFMutableDictionaryRef dict =
          (CFMutableDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
      CFDictionarySetValue(dict, kCMSampleAttachmentKey_DisplayImmediately,
                           kCFBooleanTrue);
      if (self.status == AVQueuedSampleBufferRenderingStatusFailed) {
        [self flush];
      }
      [self enqueueSampleBuffer:sampleBuffer];
      self.invisible = NO;
      CFRelease(sampleBuffer);
    }
    CFRelease(desc);
  }
  CFRelease(pixelBuffer);
  pthread_mutex_unlock(&_renderLock);
}

- (void)eraseCurrentContent {
  self.invisible = YES;
  [self flushAndRemoveImage];
}

- (void)setInvisible:(BOOL)invisible {
  if (_invisible != invisible) {
    _invisible = invisible;

    if ([NSThread isMainThread]) {
      self.hidden = invisible;
    } else {
      dispatch_async(dispatch_get_main_queue(), ^{
        self.hidden = invisible;
      });
    }
  }
}

- (void)setRenderDisabled:(BOOL)renderDisabled {
  pthread_mutex_lock(&_renderLock);
  _renderDisabled = renderDisabled;
  pthread_mutex_unlock(&_renderLock);
}

- (void)applicationEnterBackground:(NSNotification *)notification {
  self.renderDisabled = YES;
}

- (void)applicationEnterForeground:(NSNotification *)notification {
  self.renderDisabled = NO;
}

@end
#endif
