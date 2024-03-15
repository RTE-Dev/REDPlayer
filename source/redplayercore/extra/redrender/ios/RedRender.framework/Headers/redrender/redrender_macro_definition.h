/*
 * redrender_macro_definition.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "common/logger.h"
#include <inttypes.h>

#ifndef REDRENDER_MACRO_DEFINITION_H
#define REDRENDER_MACRO_DEFINITION_H

#define REDRENDER_PLATFORM_UNKNOWN 0
#define REDRENDER_PLATFORM_ANDROID 1
#define REDRENDER_PLATFORM_IOS 2
#define REDRENDER_PLATFORM_WIN32 3
#define REDRENDER_PLATFORM_MACOS 4

#define REDRENDER_PLATFORM REDRENDER_PLATFORM_UNKNOWN
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__) || defined(ANDROID)
#undef REDRENDER_PLATFORM
#define REDRENDER_PLATFORM REDRENDER_PLATFORM_ANDROID
#elif defined(PLATFORM_IOS) || defined(__APPLE__)
#undef REDRENDER_PLATFORM
#define REDRENDER_PLATFORM REDRENDER_PLATFORM_IOS
#elif defined(_WIN32) ||                                                       \
    defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#undef REDRENDER_PLATFORM
#define REDRENDER_PLATFORM REDRENDER_PLATFORM_WIN32
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

#endif /* REDRENDER_MACRO_DEFINITION_H */
