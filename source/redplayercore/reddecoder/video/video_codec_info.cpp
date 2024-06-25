#include "reddecoder/video/video_codec_info.h"
namespace reddecoder {

VideoCodecInfo::VideoCodecInfo(VideoCodecName name,
                               VideoCodecImplementationType type)
    : codec_name(name), implementation_type(type), ffmpeg_codec_id(0) {}

} // namespace reddecoder
