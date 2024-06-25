#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_video_output.h"

@implementation MetalVideoOutput

- (id)init {
  if (!(self = [super init])) {
    return nil;
  }
  outputTextureScale = 1.0;

  return self;
}

- (void)dealloc {
  outputTexture = nil;
  sharedcmdBuffer = nil;
}

- (BOOL)newSharedcmdBuffer:(MetalVideoCmdQueue *)metalVideoCmdQueue {
  if (self) {
    sharedcmdBuffer = [metalVideoCmdQueue newCmdBuffer];
    if (sharedcmdBuffer) {
      return YES;
    }
  }
  return NO;
}

- (void)releaseSharedcmdBuffer {
  sharedcmdBuffer = nil;
}

@end
#endif
