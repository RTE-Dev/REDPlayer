#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/dict.h"
#ifdef __cplusplus
}
#endif

#define REDSOURCE_NS_BEGIN namespace redsource {
#define REDSOURCE_NS_END }

REDSOURCE_NS_BEGIN
#define SOURCE_LOG_TAG "redsource"
enum class ExtractorType {
  FFMpeg = 0, // Default
  HLS         // Not Support Yet
};
struct FFMpegOpt {
  AVDictionary *format_opts;
  AVDictionary *codec_opts;
};
REDSOURCE_NS_END
