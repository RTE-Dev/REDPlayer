/*
 * redrender_buffer.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef REDRENDER_BUFFER_H
#define REDRENDER_BUFFER_H

#include "../redrender_macro_definition.h"
#include "../video/video_macro_definition.h"
#include <cstdint>
#include <memory>

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_ANDROID
//#include <EGL/egl.h>
//#include <EGL/eglext.h>
//#include <EGL/eglplatform.h>

#elif REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
#import <CoreVideo/CoreVideo.h>
//#import <OpenGLES/EAGL.h>
//#import <OpenGLES/ES2/gl.h>
//#import <OpenGLES/ES2/glext.h>

#endif

NS_REDRENDER_BEGIN

typedef enum RedRenderBufferType {
  RedRenderBufferUnknown = 0,
  RedRenderBufferVideoFrame,
  RedRenderBufferAudioFrame,
} RedRenderBufferType;

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

typedef struct AudioFrameMetaData {
  // todo
} AudioFrameMetaData;

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

typedef struct VideoHisiMediaBufferContext {
  void *media_buffer;
  int decoder_serial;
  void *decoder;
  void *opaque;
  void (*release_output_buffer)(struct VideoHisiMediaBufferContext *context,
                                bool render);
} VideoHisiMediaBufferContext;

typedef struct VideoMediaCodecBufferProxy {
  int buffer_id;
  int buffer_index;
  int acodec_serial;
  VideoMediaCodecBufferInfo buffer_info;
  VideoMediaCodecBufferContext od_buffer_ctx;
} VideoMediaCodecBufferProxy;

typedef struct VideoHisiMediaBufferProxy {
  VideoHisiMediaBufferContext od_buffer_ctx;
} VideoHisiMediaBufferProxy;

//// filter manager base class
// class RedRenderBuffer {
// public:
////    RedRenderBuffer();
//    RedRenderBuffer(RedRenderBufferType bufferType);
//    ~RedRenderBuffer();
//    void initBufferType(RedRenderBufferType bufferType);
//    VideoFrameMetaData* getVideoFrameMeta() const;
//    AudioFrameMetaData* getAudioFrameMeta() const;
// private:
//    std::unique_ptr<VideoFrameMetaData> _videoFrameMetaData;
//    std::unique_ptr<AudioFrameMetaData> _audioFrameMetaData;
//
//    RedRenderBufferType _bufferType;
//    uint8_t* _data;
//    uint64_t _size;
//};

NS_REDRENDER_END

#endif /* REDRENDER_BUFFER_H */
