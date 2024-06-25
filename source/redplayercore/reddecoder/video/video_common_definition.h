#pragma once

#if defined(__ANDROID__)
#include <android/native_window.h>
#endif

#if defined(__HARMONY__)
#include <multimedia/player_framework/native_avcodec_base.h>
#endif

namespace reddecoder {
enum class VideoCodecError {
  kNoError = 0,
  kInvalidParameter = 1,
  kInitError = 2,
  kUninited = 3,
  kInternalError = 4,
  kEOF = 5,
  kTryAgain = 6,
  kNotSupported = 7,
  kFallbackSoftware = 8,
  kBadData = 9,
  kNeedIFrame = 10,
  kAllocateBufferError = 11,
  kDecoderErrorStatus = 12,
  kPTSReorderError = 13,
};

enum class PixelFormat {
  kUnknow = 0,
  kYUV420 = 1,
  kVTBBuffer = 2,
  kMediaCodecBuffer = 3,
  kYUVJ420P = 4,
  kYUV420P10LE = 5,
  kHarmonyVideoDecoderBuffer = 6,
};

enum class VideoFormatDescType {
  kUnknow = 0,
  kSPS = 1,
  kPPS = 2,
  kVPS = 3,
  kExtraData = 4,
};

struct HardWareContext {
  virtual ~HardWareContext() = default;
};

#if defined(__ANDROID__)
struct AndroidHardWareContext : public HardWareContext {
  AndroidHardWareContext(ANativeWindow *window) : natvie_window(window) {}
  ANativeWindow *natvie_window = nullptr;
};
#endif

#if defined(__HARMONY__)
struct HarmonyHardWareContext : public HardWareContext {
  HarmonyHardWareContext(OHNativeWindow *window) : natvie_window(window) {}
  OHNativeWindow *natvie_window = nullptr;
};
#endif
} // namespace reddecoder
