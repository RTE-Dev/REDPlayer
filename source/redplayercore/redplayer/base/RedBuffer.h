#pragma once

#include "RedBase.h"

REDPLAYER_NS_BEGIN;

class CGlobalBuffer {
public:
  CGlobalBuffer() = default;
  ~CGlobalBuffer();
  void free();

  CGlobalBuffer(const CGlobalBuffer &buffer) = delete;
  CGlobalBuffer &operator=(const CGlobalBuffer &buffer) = delete;
  CGlobalBuffer(CGlobalBuffer &&buffer) = delete;
  CGlobalBuffer &operator=(CGlobalBuffer &&buffer) = delete;

  static const int MAX_PALANARS = 8;

  struct MediaCodecBufferContext {
    int buffer_index;
    void *media_codec;
    int decoder_serial;
    void *decoder;
    void *opaque;
    void (*release_output_buffer)(MediaCodecBufferContext *context,
                                  bool render);
  };

  struct FFmpegBufferContext {
    void *av_frame;
    void (*release_av_frame)(FFmpegBufferContext *context);
    void *opaque;
  };

  struct VideoToolBufferContext {
    void *buffer = nullptr;
  };

  enum PixelFormat {
    kUnknow = 0,
    kYUV420 = 1,
    kVTBBuffer = 2,
    kMediaCodecBuffer = 3,
    kYUVJ420P = 4,
    kYUV420P10LE = 5,
    kAudio = 7,
  };

public:
  int format{0};
  int serial{-1};
  int64_t datasize{0};
  int64_t dts{0};
  double pts{0};
  uint8_t *data{nullptr};
  short *audioBuf{nullptr}; // soundtouch buf

  int width{0};
  int height{0};
  int key_frame{0};
  int yStride{0}; // stride of Y data buffer
  int uStride{0}; // stride of U data buffer
  int vStride{0}; // stride of V data buffer
  uint8_t *yBuffer{nullptr};
  uint8_t *uBuffer{nullptr};
  uint8_t *vBuffer{nullptr};
  void *opaque{nullptr};
  PixelFormat pixel_format{kUnknow};

  /**
   * number of audio samples (per channel) described by this frame
   */
  int nb_samples{0};
  int sample_rate{0}; // Sample rate of the audio data
  int num_channels{0};
  uint8_t *channel[MAX_PALANARS];
  uint64_t channel_layout{0}; // Channel layout of the audio data
};

REDPLAYER_NS_END;
