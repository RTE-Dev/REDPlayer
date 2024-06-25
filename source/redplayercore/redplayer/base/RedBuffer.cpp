#include "RedBuffer.h"
#include "RedLog.h"
#include "reddecoder/video/video_decoder/video_decoder.h"

#if defined(__APPLE__)
#import <CoreVideo/CoreVideo.h>
#endif

#define TAG "RedBuffer"

REDPLAYER_NS_BEGIN;

CGlobalBuffer::~CGlobalBuffer() { free(); }

void CGlobalBuffer::free() {
  if (data) {
    delete[] data;
    data = nullptr;
  }
  if (audioBuf) {
    delete[] audioBuf;
    audioBuf = nullptr;
  }
  if (opaque) {
    switch (pixel_format) {
    case kMediaCodecBuffer: {
      MediaCodecBufferContext *ctx =
          reinterpret_cast<MediaCodecBufferContext *>(opaque);
      ctx->release_output_buffer(ctx, false);
      delete reinterpret_cast<MediaCodecBufferContext *>(opaque);
      break;
    }
    case kVTBBuffer: {
#if defined(__APPLE__)
      if (opaque &&
          (reinterpret_cast<VideoToolBufferContext *>(opaque))->buffer) {
        CVBufferRelease(
            (CVPixelBufferRef)(reinterpret_cast<VideoToolBufferContext *>(
                                   opaque))
                ->buffer);
        delete reinterpret_cast<VideoToolBufferContext *>(opaque);
        opaque = nullptr;
      }
#endif
      break;
    }
    case kYUV420:
    case kYUVJ420P:
    case kYUV420P10LE:
    case kAudio: {
      FFmpegBufferContext *ctx =
          reinterpret_cast<FFmpegBufferContext *>(opaque);
      ctx->release_av_frame(ctx);
      delete reinterpret_cast<FFmpegBufferContext *>(opaque);
      break;
    }
    case kHarmonyVideoDecoderBuffer: {
      HarmonyMediaBufferContext *ctx =
          reinterpret_cast<HarmonyMediaBufferContext *>(opaque);
      ctx->release_output_buffer(ctx, false);
      delete reinterpret_cast<HarmonyMediaBufferContext *>(opaque);
      break;
    }

    default:
      AV_LOGW(
          TAG,
          "buffer %p opaque %p unknow pixel_format %d, may cause memleak!\n",
          this, opaque, pixel_format);
      break;
    }
    opaque = nullptr;
  }
  for (int i = 0; i < MAX_PALANARS; ++i) {
    channel[i] = nullptr;
  }
  yBuffer = nullptr;
  uBuffer = nullptr;
  vBuffer = nullptr;
}

REDPLAYER_NS_END;
