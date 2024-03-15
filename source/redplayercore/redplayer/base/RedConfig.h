#pragma once

#include "RedDef.h"
#include "RedDict.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avutil.h"
#include "libavutil/dict.h"
#ifdef __cplusplus
}
#endif

#define CONFIG_OFFSET(x) offsetof(PlayerConfig, x)
#define CONFIG_INT(default__, min__, max__)                                    \
  .type = kTypeInt32, {.i64 = default__}, .min = min__, .max = max__

#define CONFIG_INT64(default__, min__, max__)                                  \
  .type = kTypeInt64, {.i64 = default__}, .min = min__, .max = max__

#define CONFIG_DOUBLE(default__, min__, max__)                                 \
  .type = kTypeDouble, {.dbl = default__}, .min = min__, .max = max__

#define CONFIG_CONST(default__)                                                \
  .type = kTypeConst, {.i64 = default__}, .min = INT_MIN, .max = INT_MAX

#define CONFIG_STR(default__)                                                  \
  .type = kTypeString, {.str = default__}, .min = 0, .max = 0

REDPLAYER_NS_BEGIN;

enum CfgType {
  cfgTypeFormat = 1,
  cfgTypeCodec = 2,
  cfgTypeSws = 3,
  cfgTypePlayer = 4,
  cfgTypeSwr = 5
};

typedef struct FFDemuxCacheControl {
  int max_buffer_size;
  int high_water_mark_in_bytes;

  int first_high_water_mark_in_ms;
  int next_high_water_mark_in_ms;
  int last_high_water_mark_in_ms;
  int current_high_water_mark_in_ms;
} FFDemuxCacheControl;

struct PlayerConfig {
  int32_t audio_disable;
  int32_t video_disable;
  int32_t loop;
  int32_t infinite_buffer;
  int32_t framedrop;
  int64_t seek_at_start;
  int32_t max_fps;
  int32_t start_on_prepared;
  int32_t pictq_size;
  int32_t packet_buffering;
  int32_t enable_accurate_seek;
  int32_t accurate_seek_timeout;
  int32_t video_hdr_enable;
  int32_t videotoolbox;
  int32_t mediacodec_auto_rotate;
  int32_t is_input_json;
  int32_t enable_ndkvdec;
  int32_t vtb_max_error_count;
  FFDemuxCacheControl dcc;
};

struct RedCfg {
  const char *name;
  const char *help;
  int offset;
  ValType type;
  union {
    int64_t i64;
    double dbl;
    const char *str;
  } defaultVal;
  double min;
  double max;
};

static const RedCfg allCfg[] = {
    {"an", "disable audio", CONFIG_OFFSET(audio_disable), CONFIG_INT(0, 0, 1)},
    {"vn", "disable video", CONFIG_OFFSET(video_disable), CONFIG_INT(0, 0, 1)},
    {"loop", "set number of times the playback shall be looped",
     CONFIG_OFFSET(loop), CONFIG_INT(1, INT_MIN, INT_MAX)},
    {"infbuf",
     "don't limit the input buffer size (useful with realtime streams)",
     CONFIG_OFFSET(infinite_buffer), CONFIG_INT(0, 0, 1)},
    {"framedrop", "drop frames when cpu is too slow", CONFIG_OFFSET(framedrop),
     CONFIG_INT(0, -1, 120)},
    {"seek-at-start", "set offset of player should be seeked",
     CONFIG_OFFSET(seek_at_start), CONFIG_INT64(0, 0, INT_MAX)},
    {"max-fps",
     "drop frames in video whose fps is greater than max-fps while using "
     "hardward decoder",
     CONFIG_OFFSET(max_fps), CONFIG_INT(61, -1, 121)},
    {"start-on-prepared", "automatically start playing on prepared",
     CONFIG_OFFSET(start_on_prepared), CONFIG_INT(1, 0, 1)},
    {"video-pictq-size", "max picture queue frame count",
     CONFIG_OFFSET(pictq_size),
     CONFIG_INT(VIDEO_PICTURE_QUEUE_SIZE_DEFAULT, VIDEO_PICTURE_QUEUE_SIZE_MIN,
                VIDEO_PICTURE_QUEUE_SIZE_MAX)},
    {"max-buffer-size", "max buffer size should be pre-read",
     CONFIG_OFFSET(dcc.max_buffer_size),
     CONFIG_INT(MAX_QUEUE_SIZE, 0, MAX_QUEUE_SIZE)},
    {"first-high-water-mark-ms", "first chance to wakeup read_thread",
     CONFIG_OFFSET(dcc.first_high_water_mark_in_ms),
     CONFIG_INT(DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS,
                DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS,
                DEFAULT_LAST_HIGH_WATER_MARK_IN_MS)},
    {"next-high-water-mark-ms", "second chance to wakeup read_thread",
     CONFIG_OFFSET(dcc.next_high_water_mark_in_ms),
     CONFIG_INT(DEFAULT_NEXT_HIGH_WATER_MARK_IN_MS,
                DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS,
                DEFAULT_LAST_HIGH_WATER_MARK_IN_MS)},
    {"last-high-water-mark-ms", "last chance to wakeup read_thread",
     CONFIG_OFFSET(dcc.last_high_water_mark_in_ms),
     CONFIG_INT(DEFAULT_LAST_HIGH_WATER_MARK_IN_MS,
                DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS,
                DEFAULT_LAST_HIGH_WATER_MARK_IN_MS)},
    {"packet-buffering",
     "pause output until enough packets have been read after stalling",
     CONFIG_OFFSET(packet_buffering), CONFIG_INT(1, 0, 1)},
    {"enable-accurate-seek", "enable accurate seek",
     CONFIG_OFFSET(enable_accurate_seek), CONFIG_INT(1, 0, 1)},
    {"accurate-seek-timeout", "accurate seek timeout",
     CONFIG_OFFSET(accurate_seek_timeout),
     CONFIG_INT(MAX_ACCURATE_SEEK_TIMEOUT, 0, MAX_ACCURATE_SEEK_TIMEOUT)},
    {"enable-video-hdr", "enable video hdr", CONFIG_OFFSET(video_hdr_enable),
     CONFIG_INT(0, 0, 1)},
    {"is-input-json", "is input json", CONFIG_OFFSET(is_input_json),
     CONFIG_INT(0, 0, 2)},

    // iOS only options
    {"videotoolbox", "VideoToolbox: enable", CONFIG_OFFSET(videotoolbox),
     CONFIG_INT(0, 0, 1)},
    {"videotoolbox-max-error-count", "VideoToolbox: max error count",
     CONFIG_OFFSET(vtb_max_error_count), CONFIG_INT(5, 0, INT_MAX)},

    // Android only options
    {"mediacodec-auto-rotate",
     "MediaCodec: auto rotate frame depending on meta",
     CONFIG_OFFSET(mediacodec_auto_rotate), CONFIG_INT(0, 0, 1)},
    {"enable-ndkvdec", "use ndk mediacodec", CONFIG_OFFSET(enable_ndkvdec),
     CONFIG_INT(0, 0, 1)},
    {NULL}};

class RedPlayerConfig {
public:
  RedPlayerConfig();
  ~RedPlayerConfig();
  void reset();
  PlayerConfig *operator->();
  PlayerConfig *get();

private:
  DISALLOW_EVIL_CONSTRUCTORS(RedPlayerConfig);
  std::mutex mMutex;
  PlayerConfig *mConfig{nullptr};
};

struct CoreGeneralConfig {
  sp<RedPlayerConfig> playerConfig;
  AVDictionary *formatConfig{nullptr};
  AVDictionary *codecConfig{nullptr};
  AVDictionary *swsConfig{nullptr};
  AVDictionary *swrConfig{nullptr};
};

REDPLAYER_NS_END;
