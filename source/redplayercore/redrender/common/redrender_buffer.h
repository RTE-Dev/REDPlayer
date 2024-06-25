#pragma once

#include <cstdint>
#include <memory>

#include "../redrender_macro_definition.h"
#include "../video/video_macro_definition.h"

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import <CoreVideo/CoreVideo.h>
#endif

NS_REDRENDER_BEGIN

typedef struct VideoFrameMetaData {
  uint8_t *pitches[3];
  int linesize[3];
  int32_t stride;

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
  CVPixelBufferRef pixel_buffer;
#endif
  int layerWidth;
  int layerHeight;
  int frameWidth;
  int frameHeight;

  VRPixelFormat pixel_format;
  VRAVColorPrimaries color_primaries;
  VRAVColorTrC color_trc;
  VRAVColorSpace color_space;
  VRAVColorRange color_range;
  GravityResizeAspectRatio aspectRatio;
  RotationMode rotationMode;
  SARRational sar;
} VideoFrameMetaData;

typedef struct VideoMediaCodecBufferInfo {
  int32_t offset;
  int32_t size;
  int64_t presentationTimeUs;
  uint32_t flags;
} VideoMediaCodecBufferInfo;

typedef struct VideoMediaCodecBufferContext {
  int buffer_index;
  void *media_codec;
  int decoder_serial;
  void *decoder;
  void *opaque;
  void (*release_output_buffer)(struct VideoMediaCodecBufferContext *context,
                                bool render);
} VideoMediaCodecBufferContext;

typedef struct VideoHarmonyDecoderBufferInfo {
  int32_t offset;
  int32_t size;
  int64_t presentationTimeUs;
  uint32_t flags;
} VideoHarmonyDecoderBufferInfo;

typedef struct VideoHarmonyDecoderBufferContext {
  int buffer_index;
  void *video_decoder;
  int decoder_serial;
  void *decoder;
  void *opaque;
  void (*release_output_buffer)(
      struct VideoHarmonyDecoderBufferContext *context, bool render);
} VideoHarmonyDecoderBufferContext;

NS_REDRENDER_END
