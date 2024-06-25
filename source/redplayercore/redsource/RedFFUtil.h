#pragma once
#include "RedSourceCommon.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "libavutil/display.h"
#include "libavutil/eval.h"
#include "libavutil/opt.h"
#ifdef __cplusplus
}
#endif
REDSOURCE_NS_BEGIN
int64_t get_bit_rate(AVCodecParameters *codecpar);
double get_rotation(AVStream *st);
AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts);
AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st,
                                AVCodec *codec);
REDSOURCE_NS_END
