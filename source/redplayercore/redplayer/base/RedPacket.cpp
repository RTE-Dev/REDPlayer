#include "RedPacket.h"

REDPLAYER_NS_BEGIN;

static inline int ff_get_nal_units_type(const uint8_t *const data) {
  return data[4] & 0x1f;
}

static inline int ff_get_hevc_nal_units_type(const uint8_t *const data) {
  return (data[4] & 0x7E) >> 1;
}

static inline uint32_t bytesToInt(uint8_t *src) {
  uint32_t value;
  value = (uint32_t)((src[0] & 0xFF) << 24 | (src[1] & 0xFF) << 16 |
                     (src[2] & 0xFF) << 8 | (src[3] & 0xFF));
  return value;
}

RedAvPacket::RedAvPacket(AVPacket *pkt) {
  pkt_ = av_packet_alloc();
  av_init_packet(pkt_);
  av_packet_ref(pkt_, pkt);
}

RedAvPacket::RedAvPacket(AVPacket *pkt, int serial) : RedAvPacket(pkt) {
  serial_ = serial;
}

RedAvPacket::RedAvPacket(PktType type) : type_(type) {}

RedAvPacket::~RedAvPacket() {
  if (pkt_) {
    av_packet_free(&pkt_);
  }
}

bool RedAvPacket::IsFlushPacket() { return type_ == PKT_TYPE_FLUSH; }

bool RedAvPacket::IsEofPacket() { return type_ == PKT_TYPE_EOF; }

bool RedAvPacket::IsKeyPacket() {
  if (pkt_ && pkt_->flags & AV_PKT_FLAG_KEY) {
    return true;
  } else {
    return false;
  }
}

bool RedAvPacket::IsIdrPacket(bool is_hevc) {
  int state = -1;

  if (pkt_ && pkt_->data && pkt_->size >= 5) {
    int offset = 0;
    while (offset >= 0 && offset + 5 <= pkt_->size) {
      void *nal_start = pkt_->data + offset;
      if (is_hevc) {
        state = ff_get_hevc_nal_units_type((const uint8_t *const)nal_start);
        if (state == HEVC_NAL_BLA_W_LP || state == HEVC_NAL_BLA_W_RADL ||
            state == HEVC_NAL_BLA_N_LP || state == HEVC_NAL_IDR_W_RADL ||
            state == HEVC_NAL_IDR_N_LP || state == HEVC_NAL_CRA_NUT) {
          return true;
        }
      } else {
        state = ff_get_nal_units_type((const uint8_t *const)nal_start);
        if (state == NAL_IDR_SLICE) {
          return true;
        }
      }
      offset += (bytesToInt(reinterpret_cast<uint8_t *>(nal_start)) + 4);
    }
  }
  return false;
}

bool RedAvPacket::IsKeyOrIdrPacket(bool is_idr, bool is_hevc) {
  if (is_idr) {
    return IsIdrPacket(is_hevc);
  } else {
    return IsKeyPacket();
  }
}

AVPacket *RedAvPacket::GetAVPacket() { return pkt_; }

int RedAvPacket::GetSerial() { return serial_; }

REDPLAYER_NS_END;
