#include "./audio_render_factory.h"
#ifdef __ANDROID__
#include "android/audio_track_render.h"
#endif
#ifdef __APPLE__
#include "ios/audio_queue_render.h"
#endif
#ifdef __HARMONY__
#include "harmony/harmony_audio_render.h"
#endif

USING_NS_REDRENDER_AUDIO
std::unique_ptr<IAudioRender>
AudioRenderFactory::CreateAudioRender(const int &session_id) {
#ifdef __ANDROID__
  return std::unique_ptr<IAudioRender>(new AudioTrackRender(session_id));
#endif
#ifdef __APPLE__
  return std::unique_ptr<IAudioRender>(new AudioQueueRender(session_id));
#endif
#ifdef __HARMONY__
  return std::unique_ptr<IAudioRender>(new HarmonyAudioRender(session_id));
#endif
  return nullptr;
}
