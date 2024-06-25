#include "reddecoder/video/video_common/format_convert_helper.h"

#include <memory>

int FormatConvertHelper::convert_hevc_nal_units(
    const uint8_t *p_buf, size_t i_buf_size, uint8_t *p_out_buf,
    size_t i_out_buf_size, size_t *p_sps_pps_size, size_t *p_nal_size) {
  int i, num_arrays;
  const uint8_t *p_end = p_buf + i_buf_size;
  uint32_t i_sps_pps_size = 0;

  if (i_buf_size <= 3 || (!p_buf[0] && !p_buf[1] && p_buf[2] <= 1))
    return -1;

  if (p_end - p_buf < 23) {
    return -1;
  }

  p_buf += 21;

  if (p_nal_size)
    *p_nal_size = (*p_buf & 0x03) + 1;
  p_buf++;

  num_arrays = *p_buf++;

  for (i = 0; i < num_arrays; i++) {
    int type, cnt, j;

    if (p_end - p_buf < 3) {
      return -1;
    }
    type = *(p_buf++) & 0x3f;
    (void)(type);

    cnt = p_buf[0] << 8 | p_buf[1];
    p_buf += 2;

    for (j = 0; j < cnt; j++) {
      int i_nal_size;

      if (p_end - p_buf < 2) {
        return -1;
      }

      i_nal_size = p_buf[0] << 8 | p_buf[1];
      p_buf += 2;

      if (i_nal_size < 0 || p_end - p_buf < i_nal_size) {
        return -1;
      }

      if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size) {
        return -1;
      }

      p_out_buf[i_sps_pps_size++] = 0;
      p_out_buf[i_sps_pps_size++] = 0;
      p_out_buf[i_sps_pps_size++] = 0;
      p_out_buf[i_sps_pps_size++] = 1;

      memcpy(p_out_buf + i_sps_pps_size, p_buf, i_nal_size);
      p_buf += i_nal_size;

      i_sps_pps_size += i_nal_size;
    }
  }

  *p_sps_pps_size = i_sps_pps_size;

  return 0;
}

int FormatConvertHelper::convert_sps_pps(const uint8_t *p_buf,
                                         size_t i_buf_size, uint8_t *p_out_buf,
                                         size_t i_out_buf_size,
                                         size_t *p_sps_pps_size,
                                         size_t *p_nal_size) {
  uint32_t i_data_size = i_buf_size, i_nal_size, i_sps_pps_size = 0;
  unsigned int i_loop_end;

  if (i_data_size < 7) {
    return -1;
  }

  if (p_nal_size)
    *p_nal_size = (p_buf[4] & 0x03) + 1;
  p_buf += 5;
  i_data_size -= 5;

  for (unsigned int j = 0; j < 2; j++) {
    if (i_data_size < 1) {
      return -1;
    }
    i_loop_end = p_buf[0] & (j == 0 ? 0x1f : 0xff);
    p_buf++;
    i_data_size--;

    for (unsigned int i = 0; i < i_loop_end; i++) {
      if (i_data_size < 2) {
        return -1;
      }

      i_nal_size = (p_buf[0] << 8) | p_buf[1];
      p_buf += 2;
      i_data_size -= 2;

      if (i_data_size < i_nal_size) {
        return -1;
      }
      if (i_sps_pps_size + 4 + i_nal_size > i_out_buf_size) {
        return -1;
      }

      p_out_buf[i_sps_pps_size++] = 0;
      p_out_buf[i_sps_pps_size++] = 0;
      p_out_buf[i_sps_pps_size++] = 0;
      p_out_buf[i_sps_pps_size++] = 1;

      memcpy(p_out_buf + i_sps_pps_size, p_buf, i_nal_size);
      i_sps_pps_size += i_nal_size;

      p_buf += i_nal_size;
      i_data_size -= i_nal_size;
    }
  }

  *p_sps_pps_size = i_sps_pps_size;

  return 0;
}

void FormatConvertHelper::convert_avcc_to_annexb(uint8_t *data, size_t size) {
  static uint8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
  uint8_t *head = data;
  while (data < head + size) {
    int nal_len = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    if (nal_len > 0) {
      memcpy(data, start_code, 4);
    }
    data = data + 4 + nal_len;
  }
}

void FormatConvertHelper::convert_h2645_to_annexb(uint8_t *p_buf, size_t i_len,
                                                  size_t i_nal_size,
                                                  H2645ConvertState *state) {
  if (i_nal_size < 3 || i_nal_size > 4)
    return;

  while (i_len > 0) {
    if (state->nal_pos < i_nal_size) {
      unsigned int i;
      for (i = 0; state->nal_pos < i_nal_size && i < i_len;
           i++, state->nal_pos++) {
        state->nal_len = (state->nal_len << 8) | p_buf[i];
        p_buf[i] = 0;
      }
      if (state->nal_pos < i_nal_size)
        return;
      p_buf[i - 1] = 1;
      p_buf += i;
      i_len -= i;
    }
    if (state->nal_len > INT_MAX)
      return;
    if (state->nal_len > i_len) {
      state->nal_len -= i_len;
      return;
    } else {
      p_buf += state->nal_len;
      i_len -= state->nal_len;
      state->nal_len = 0;
      state->nal_pos = 0;
    }
  }
}
