#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../video_inc_internal.h"
#import "metal_video_cmd_queue.h"
#import "metal_video_input.h"
#import "metal_video_texture.h"

@interface MetalVideoOutput : NSObject {
  MetalVideoTexture *outputTexture;
  float outputTextureScale;
  id<MTLCommandBuffer> sharedcmdBuffer;
}

- (BOOL)newSharedcmdBuffer:(MetalVideoCmdQueue *)metalVideoCmdQueue;
- (void)releaseSharedcmdBuffer;

@end
#endif
