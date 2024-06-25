#pragma once

#include <cstddef>
#include <cstdint>

class FormatConvertHelper {
public:
  struct H2645ConvertState {
    uint32_t nal_len;
    uint32_t nal_pos;
  };
  static int convert_sps_pps(const uint8_t *p_buf, size_t i_buf_size,
                             uint8_t *p_out_buf, size_t i_out_buf_size,
                             size_t *p_sps_pps_size, size_t *p_nal_size);

  static int convert_hevc_nal_units(const uint8_t *p_buf, size_t i_buf_size,
                                    uint8_t *p_out_buf, size_t i_out_buf_size,
                                    size_t *p_sps_pps_size, size_t *p_nal_size);

  static void convert_avcc_to_annexb(uint8_t *data, size_t size);

  static void convert_h2645_to_annexb(uint8_t *p_buf, size_t i_len,
                                      size_t i_nal_size,
                                      H2645ConvertState *state);
};
