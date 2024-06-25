#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import "metal_video_texture.h"
#import "metal_video_cmd_queue.h"

@implementation MetalVideoTexture

- (id)init:(id<MTLDevice>)device
      sessionID:(uint32_t)sessionID
          width:(uint32_t)imageWidth
         height:(uint32_t)imageHeight
    pixelFormat:(MTLPixelFormat)pixelFormat {
  if (!(self = [super init]) || device == nil || imageWidth <= 0 ||
      imageHeight <= 0) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] MetalVideoTexture init error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }

  _sessionID = sessionID;
  _width = imageWidth;
  _height = imageHeight;
  _pixelFormat = pixelFormat;
  _inputTextureIndex = 0;
  _rotationMode = RedRender::NoRotation;
  _texture = nil;

  MTLTextureDescriptor *textureDescriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:_pixelFormat
                                                         width:_width
                                                        height:_height
                                                     mipmapped:NO];
  if (!textureDescriptor) {
    AV_LOGE_ID(LOG_TAG, sessionID,
               "[%s:%d] texture2DDescriptorWithPixelFormat error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }
  textureDescriptor.usage = MTLTextureUsageShaderRead |
                            MTLTextureUsageShaderWrite |
                            MTLTextureUsageRenderTarget;
  _textureType = textureDescriptor.textureType;
  _texture = [device newTextureWithDescriptor:textureDescriptor];
  if (!_texture) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] newTextureWithDescriptor error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }

  return self;
}

- (id)initWithPixelBuffer:(CVMetalTextureCacheRef)textureCache
              pixelBuffer:(CVPixelBufferRef)pixelBuffer
               planeIndex:(int)planeIndex
                sessionID:(uint32_t)sessionID
              pixelFormat:(MTLPixelFormat)pixelFormat {
  if (!(self = [super init]) || pixelBuffer == nil) {
    AV_LOGE_ID(LOG_TAG, sessionID, "[%s:%d] MetalVideoTexture init error .\n",
               __FUNCTION__, __LINE__);
    return nil;
  }

  _sessionID = sessionID;
  _pixelFormat = pixelFormat;
  _inputTextureIndex = 0;
  _rotationMode = RedRender::NoRotation;
  _texture = nil;

  _width = (uint32_t)CVPixelBufferGetWidthOfPlane(pixelBuffer, planeIndex);
  _height = (uint32_t)CVPixelBufferGetHeightOfPlane(pixelBuffer, planeIndex);

  CVMetalTextureRef texture = nil;
  CVReturn status = CVMetalTextureCacheCreateTextureFromImage(
      nil, textureCache, pixelBuffer, nil, pixelFormat, _width, _height,
      planeIndex, &texture);
  if (status == kCVReturnSuccess) {
    _texture = CVMetalTextureGetTexture(texture);
    CFRelease(texture);
    texture = nil;
    return self;
  } else {
    AV_LOGE_ID(
        LOG_TAG, sessionID,
        "[%s:%d] MetalVideoTexture init error:%d, _width:%d, _height:%d .\n",
        __FUNCTION__, __LINE__, (int32_t)status, _width, _height);
    return nil;
  }
}

- (void)dealloc {
  _texture = nil;
}

@end
#endif
