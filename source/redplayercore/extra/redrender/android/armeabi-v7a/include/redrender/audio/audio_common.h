/*
 * @Author: chengyifeng
 * @Date: 2022-11-23 19:55:49
 * @LastEditors: chengyifeng
 * @LastEditTime: 2023-09-16 14:49:21
 * @Description: 请填写简介
 */

#pragma once
#include "../redrender_macro_definition.h"
#include <cstdint>

NS_REDRENDER_AUDIO_BEGIN
constexpr int KAudioMaxCallbacksPerSec = 30;
constexpr int KAudioQueueNumberBuffers = 3;
constexpr int KAudioMinBuferSize = 512;

enum class AudioFormat {
  kAudioInvalid = 0,
  kAudioS16Sys = 1,
  kAudioS32Sys = 2,
  kAudioF32Sys = 3,
  kAudioU8 = 4,
};

NS_REDRENDER_AUDIO_END
