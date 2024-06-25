#pragma once

#include "common/logger.h"
#include <inttypes.h>

#define REDRENDER_PLATFORM_UNKNOWN 0
#define REDRENDER_PLATFORM_ANDROID 1
#define REDRENDER_PLATFORM_IOS 2
#define REDRENDER_PLATFORM_HARMONY 5

#define REDRENDER_PLATFORM REDRENDER_PLATFORM_UNKNOWN
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__) || defined(ANDROID)
#undef REDRENDER_PLATFORM
#define REDRENDER_PLATFORM REDRENDER_PLATFORM_ANDROID
#elif defined(PLATFORM_IOS) || defined(__APPLE__)
#undef REDRENDER_PLATFORM
#define REDRENDER_PLATFORM REDRENDER_PLATFORM_IOS
#elif defined(__HARMONY__)
#undef REDRENDER_PLATFORM
#define REDRENDER_PLATFORM REDRENDER_PLATFORM_HARMONY
#endif

#define NS_REDRENDER_BEGIN namespace RedRender {
#define NS_REDRENDER_END }
#define USING_NS_REDRENDER using namespace RedRender;

#define AUDIO_LOG_TAG "audiorender"
#define NS_REDRENDER_AUDIO_BEGIN                                               \
  namespace redrender {                                                        \
  namespace audio {
#define NS_REDRENDER_AUDIO_END                                                 \
  }                                                                            \
  }
#define USING_NS_REDRENDER_AUDIO using namespace redrender::audio;
