#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

class H264FormatParser {
 public:
  struct H264Info {
    int max_ref_frames = 0;
    int level = 0;
    int profile = 0;
    bool is_interlaced = false;
  };
  static std::unique_ptr<H264FormatParser::H264Info>
  get_h264_info_from_extradata(uint8_t *extradata, uint32_t extrasize);
  static void parseh264_sps(uint8_t *sps, uint32_t sps_size, int *level,
                            int *profile, bool *interlaced,
                            int32_t *max_ref_frames);
};