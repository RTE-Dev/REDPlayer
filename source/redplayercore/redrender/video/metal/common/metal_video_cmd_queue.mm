#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_video_cmd_queue.h"

@interface MetalVideoCmdQueue () {
  NSUInteger maxCmdBuffersToProcessing; // The maximum number of command buffers
                                        // in flight.
}
@end

@implementation MetalVideoCmdQueue

+ (id)getCmdQueueInstance {
  return [[self alloc] init];
}

- (id<MTLCommandBuffer>)newCmdBuffer {
  return [_commandQueue commandBuffer];
}

#pragma mark - Private Method
- (id)init {
  if (!(self = [super init])) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] MetalVideoCmdQueue init error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }
  _device = MTLCreateSystemDefaultDevice();
  maxCmdBuffersToProcessing = 3;
  _cmdBufferProcessSemaphore =
      dispatch_semaphore_create(maxCmdBuffersToProcessing);
  _commandQueue = [_device newCommandQueue];
  if (_commandQueue == nil) {
    AV_LOGE_ID(LOG_TAG, _sessionID, "[%s:%d] newCommandQueue error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }
  _commandQueue.label = @"Red Metal Command Queue";
  CVReturn ret =
      CVMetalTextureCacheCreate(nil, nil, _device, nil, &_textureCache);
  if (kCVReturnSuccess != ret) {
    AV_LOGE_ID(LOG_TAG, _sessionID,
               "[%s:%d] CVMetalTextureCacheCreate error .\n", __FUNCTION__,
               __LINE__);
    return nil;
  }

  return self;
}

- (void)dealloc {
  _device = nil;
  _commandQueue = nil;
  _cmdBufferProcessSemaphore = nil;
  if (_textureCache) {
    CVMetalTextureCacheFlush(_textureCache, 0);
    CFRelease(_textureCache);
    _textureCache = nil;
  }
}

@end
#endif
