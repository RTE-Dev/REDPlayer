#include "reddecoder/audio/audio_codec_info.h"
namespace reddecoder {

AudioCodecInfo::AudioCodecInfo(AudioCodecName name, AudioCodecProfile profile,
                               AudioCodecImplementationType type)
    : codec_name(name), profile(profile), implementation_type(type) {}

} // namespace reddecoder
