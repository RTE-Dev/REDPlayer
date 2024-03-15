#pragma once

#include "RedBase.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/packet.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#ifdef __cplusplus
}
#endif

REDPLAYER_NS_BEGIN;

#define MIN_PKT_DURATION 15

/* NAL unit types */
enum {
  NAL_SLICE = 1,
  NAL_DPA = 2,
  NAL_DPB = 3,
  NAL_DPC = 4,
  NAL_IDR_SLICE = 5,
  NAL_SEI = 6,
  NAL_SPS = 7,
  NAL_PPS = 8,
  NAL_AUD = 9,
  NAL_END_SEQUENCE = 10,
  NAL_END_STREAM = 11,
  NAL_FILLER_DATA = 12,
  NAL_SPS_EXT = 13,
  NAL_AUXILIARY_SLICE = 19,
  NAL_FF_IGNORE = 0xff0f001,
};

enum HEVCNALUnitType {
  HEVC_NAL_TRAIL_N = 0,
  HEVC_NAL_TRAIL_R = 1,
  HEVC_NAL_TSA_N = 2,
  HEVC_NAL_TSA_R = 3,
  HEVC_NAL_STSA_N = 4,
  HEVC_NAL_STSA_R = 5,
  HEVC_NAL_RADL_N = 6,
  HEVC_NAL_RADL_R = 7,
  HEVC_NAL_RASL_N = 8,
  HEVC_NAL_RASL_R = 9,
  HEVC_NAL_VCL_N10 = 10,
  HEVC_NAL_VCL_R11 = 11,
  HEVC_NAL_VCL_N12 = 12,
  HEVC_NAL_VCL_R13 = 13,
  HEVC_NAL_VCL_N14 = 14,
  HEVC_NAL_VCL_R15 = 15,
  HEVC_NAL_BLA_W_LP = 16,
  HEVC_NAL_BLA_W_RADL = 17,
  HEVC_NAL_BLA_N_LP = 18,
  HEVC_NAL_IDR_W_RADL = 19,
  HEVC_NAL_IDR_N_LP = 20,
  HEVC_NAL_CRA_NUT = 21,
  HEVC_NAL_IRAP_VCL22 = 22,
  HEVC_NAL_IRAP_VCL23 = 23,
  HEVC_NAL_RSV_VCL24 = 24,
  HEVC_NAL_RSV_VCL25 = 25,
  HEVC_NAL_RSV_VCL26 = 26,
  HEVC_NAL_RSV_VCL27 = 27,
  HEVC_NAL_RSV_VCL28 = 28,
  HEVC_NAL_RSV_VCL29 = 29,
  HEVC_NAL_RSV_VCL30 = 30,
  HEVC_NAL_RSV_VCL31 = 31,
  HEVC_NAL_VPS = 32,
  HEVC_NAL_SPS = 33,
  HEVC_NAL_PPS = 34,
  HEVC_NAL_AUD = 35,
  HEVC_NAL_EOS_NUT = 36,
  HEVC_NAL_EOB_NUT = 37,
  HEVC_NAL_FD_NUT = 38,
  HEVC_NAL_SEI_PREFIX = 39,
  HEVC_NAL_SEI_SUFFIX = 40,
};

enum PktType { PKT_TYPE_DEFAULT = 0, PKT_TYPE_FLUSH = 1, PKT_TYPE_EOF = 2 };

class RedAvPacket {
public:
  explicit RedAvPacket(AVPacket *pkt);
  explicit RedAvPacket(AVPacket *pkt, int serial);
  explicit RedAvPacket(PktType type);
  ~RedAvPacket();
  bool IsFlushPacket();
  bool IsEofPacket();
  bool IsKeyPacket();
  bool IsIdrPacket(bool is_hevc);
  bool IsKeyOrIdrPacket(bool is_idr, bool is_hevc);
  AVPacket *GetAVPacket();
  int GetSerial();
  RedAvPacket(const RedAvPacket &) = delete;
  RedAvPacket &operator=(const RedAvPacket &) = delete;
  RedAvPacket(RedAvPacket &&) = delete;
  RedAvPacket &operator=(RedAvPacket &&) = delete;

private:
  AVPacket *pkt_{nullptr};
  int serial_{0};
  PktType type_{PKT_TYPE_DEFAULT};
};

REDPLAYER_NS_END;
