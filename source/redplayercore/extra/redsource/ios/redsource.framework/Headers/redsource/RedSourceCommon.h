#pragma once
#include "RedDef.h"
#include "RedMsg.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#ifdef __cplusplus
}
#endif
namespace redsource {
#define SOURCE_LOG_TAG "redsource"
enum class ExtractorType {
  FFMpeg = 0, // Default
  HLS         // Not Support Yet
};
struct FFMpegOpt {
  AVDictionary *format_opts;
  AVDictionary *codec_opts;
};
} // namespace redsource