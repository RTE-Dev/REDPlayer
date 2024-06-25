#pragma once

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#include "../../video_inc_internal.h"
#import "metal_video_texture.h"
#import <Metal/Metal.h>

@protocol MetalVideoInput <NSObject>

- (void)setInputTexture:(MetalVideoTexture *)inputTexture
           rotationMode:(RedRender::RotationMode)rotationMode
      inputTextureIndex:(int)inputTextureIndex;
- (void)clearInputTexture;
- (bool)isPrepared;

@end
#endif
