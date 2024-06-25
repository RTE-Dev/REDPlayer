#pragma once
#include <cstdint>
#include <memory>

#include "./audio_render_interface.h"
NS_REDRENDER_AUDIO_BEGIN
class AudioRenderFactory {
public:
  static std::unique_ptr<IAudioRender>
  CreateAudioRender(const int &session_id = 0);
};
NS_REDRENDER_AUDIO_END
