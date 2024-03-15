/*
 * @Author: chengyifeng
 * @Date: 2022-11-02 16:59:42
 * @LastEditors: chengyifeng
 * @LastEditTime: 2023-01-31 15:09:48
 * @Description: 请填写简介
 */
#pragma once
#include "audio_render_interface.h"
#include <cstdint>
#include <memory>
NS_REDRENDER_AUDIO_BEGIN
class AudioRenderFactory {
public:
  static std::unique_ptr<IAudioRender>
  CreateAudioRender(const int &session_id = 0);
};
NS_REDRENDER_AUDIO_END
