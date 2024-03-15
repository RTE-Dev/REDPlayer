#pragma once

#include "RedDict.h"

// media meta
#define RED_META_KEY_FORMAT "format"
#define RED_META_KEY_DURATION_US "duration_us"
#define RED_META_KEY_START_US "start_us"
#define RED_META_KEY_BITRATE "bitrate"
#define RED_META_KEY_VIDEO_STREAM "video"
#define RED_META_KEY_AUDIO_STREAM "audio"
#define RED_META_KEY_TIMEDTEXT_STREAM "timedtext"

// stream meta
#define RED_META_KEY_TYPE "type"
#define RED_META_VAL_TYPE__VIDEO "video"
#define RED_META_VAL_TYPE__AUDIO "audio"
#define RED_META_VAL_TYPE__TIMEDTEXT "timedtext"
#define RED_META_VAL_TYPE__UNKNOWN "unknown"
#define RED_META_KEY_LANGUAGE "language"

#define RED_META_KEY_CODEC_NAME "codec_name"
#define RED_META_KEY_CODEC_PROFILE "codec_profile"
#define RED_META_KEY_CODEC_LEVEL "codec_level"
#define RED_META_KEY_CODEC_LONG_NAME "codec_long_name"
#define RED_META_KEY_CODEC_PIXEL_FORMAT "codec_pixel_format"
#define RED_META_KEY_CODEC_PROFILE_ID "codec_profile_id"

// stream: video
#define RED_META_KEY_WIDTH "width"
#define RED_META_KEY_HEIGHT "height"
#define RED_META_KEY_FPS_NUM "fps_num"
#define RED_META_KEY_FPS_DEN "fps_den"
#define RED_META_KEY_TBR_NUM "tbr_num"
#define RED_META_KEY_TBR_DEN "tbr_den"
#define RED_META_KEY_SAR_NUM "sar_num"
#define RED_META_KEY_SAR_DEN "sar_den"
#define RED_META_KEY_ROTATE "video_rotate"
#define RED_META_KEY_COLOR_PRIMARIES "colour_primaries"
#define RED_META_KEY_TRANSFER_CHARACTERISTICS "transfer_characteristics"
#define RED_META_KEY_MATRIX_COEFFICIENTS "matrix_coefficients"
// stream: audio
#define RED_META_KEY_SAMPLE_RATE "sample_rate"
#define RED_META_KEY_CHANNEL_LAYOUT "channel_layout"

// reserved for user
#define RED_META_KEY_STREAMS "streams"

class RedMeta : public RedDict {
public:
  RedMeta() = default;
  virtual ~RedMeta() = default;

  void SetCount(int count) { count_.store(count); }
  int GetCount() { return count_.load(); }

private:
  std::atomic_int count_{0};
  DISALLOW_EVIL_CONSTRUCTORS(RedMeta);
};
