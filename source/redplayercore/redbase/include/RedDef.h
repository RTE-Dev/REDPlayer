/*
 * RedDef.h
 *
 *  Created on: 2022年9月6日
 *      Author: liuhongda
 */

#pragma once
#include "RedBase.h"
#include <string>
#include <vector>

#define BUFFERING_CHECK_PER_MILLISECONDS (500)
#define FAST_BUFFERING_CHECK_PER_MILLISECONDS (50)

#define AV_NOSYNC_THRESHOLD (100.0)
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MAX_ACCURATE_SEEK_TIMEOUT (5000)
#define DEFAULT_MIN_FRAMES (50000)
#define MIN_MIN_FRAMES (2)
#define MAX_MIN_FRAMES (50000)
#define MAX_DEVIATION (60000) // 60ms
#define MAX_URL_LIST_SIZE (4096)
#define RED_TCP_READ_SAMPLE_RANGE (2000)

#define AVCODEC_MODULE_NAME "avcodec"
#define VTB_MODULE_NAME "videotoolbox"
#define MEDIACODEC_MODULE_NAME "MediaCodec"

using NotifyCallback =
    std::function<void(int, int, int, void *, void *, int, int)>;
using InjectCallback = std::function<int(void *, int, void *)>;
using LogCallback = std::function<void(int, const char *, const char *)>;

enum BufferState {
  BUFFER_IDLE = 0,
  BUFFER_LOW,
  BUFFER_HIGH,
  BUFFER_HUNGURY,
  BUFFER_FULL,
  BUFFER_UNKNOWN
};

enum PlayerState {
  MP_STATE_IDLE = 0,
  MP_STATE_INITIALIZED,
  MP_STATE_ASYNC_PREPARING,
  MP_STATE_PREPARED,
  MP_STATE_STARTED,
  MP_STATE_PAUSED,
  MP_STATE_COMPLETED,
  MP_STATE_STOPPED,
  MP_STATE_ERROR,
  MP_STATE_END,
  MP_STATE_SEEK_START,
  MP_STATE_SEEK_END,

  MP_STATE_INTERRUPT,
  MP_STATE_UNKNOWN
};

enum {
  kWhatSetDataSource = '=DaS',
  kWhatPrepare = 'prep',
  kWhatSetVideoSurface = '=VSu',
  kWhatSetAudioSink = '=AuS',
  kWhatMoreDataQueued = 'more',
  kWhatConfigPlayback = 'cfPB',
  kWhatConfigSync = 'cfSy',
  kWhatGetPlaybackSettings = 'gPbS',
  kWhatGetSyncSettings = 'gSyS',
  kWhatStart = 'strt',
  kWhatScanSources = 'scan',
  kWhatVideoNotify = 'vidN',
  kWhatAudioNotify = 'audN',
  kWhatClosedCaptionNotify = 'capN',
  kWhatRendererNotify = 'renN',
  kWhatReset = 'rset',
  kWhatNotifyTime = 'nfyT',
  kWhatSeek = 'seek',
  kWhatPause = 'paus',
  kWhatResume = 'rsme',
  kWhatStop = 'stop',
  kWhatPollDuration = 'polD',
  kWhatSourceNotify = 'srcN',
  kWhatGetTrackInfo = 'gTrI',
  kWhatGetSelectedTrack = 'gSel',
  kWhatSelectTrack = 'selT',
  kWhatGetBufferingSettings = 'gBus',
  kWhatSetBufferingSettings = 'sBuS',
  kWhatPrepareDrm = 'pDrm',
  kWhatReleaseDrm = 'rDrm',
  kWhatMediaClockNotify = 'mckN',
};

enum DiscardType {
  /* We leave some space between them for extensions (drop some
   * keyframes for intra-only or drop just some bidir frames). */
  DISCARD_NONE = -16,    ///< discard nothing
  DISCARD_DEFAULT = 0,   ///< discard useless packets like 0 size packets in avi
  DISCARD_NONREF = 8,    ///< discard all non reference
  DISCARD_BIDIR = 16,    ///< discard all bidirectional frames
  DISCARD_NONINTRA = 24, ///< discard all non intra frames
  DISCARD_NONKEY = 32,   ///< discard all frames except keyframes
  DISCARD_ALL = 48,      ///< discard all
};

/* NAL unit types */
enum NalUnitType {
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

enum HevcNalUnitType {
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

static inline bool IsHevcNalNonRef(int type) {
  switch (type) {
  case HEVC_NAL_TRAIL_N:
  case HEVC_NAL_TSA_N:
  case HEVC_NAL_STSA_N:
  case HEVC_NAL_RADL_N:
  case HEVC_NAL_RASL_N:
  case HEVC_NAL_VCL_N10:
  case HEVC_NAL_VCL_N12:
  case HEVC_NAL_VCL_N14:
    return true;
  default:
    break;
  }
  return false;
}

enum RedFourcc {
  // YUV formats
  RED_FCC_YV12 = 'YV12',
  RED_FCC_IYUV = 'IYUV',
  RED_FCC_I420 = 'I420',
  RED_FCC_I444P10LE = 'I4AL',
  RED_FCC_YUV2 = 'YUV2',
  RED_FCC_UYVY = 'UYVY',
  RED_FCC_YVYU = 'YVYU',
  RED_FCC_NV12 = 'NV12',
  // RGB formats
  RED_FCC_RV16 = 'RV16',
  RED_FCC_RV24 = 'RV24',
  RED_FCC_RV32 = 'RV32',
  // opaque formats
  RED_FCC__AMC = '_AMC',
  RED_FCC__HMB = '_HMB',
  RED_FCC__VTB = '_VTB',
  RED_FCC__GLES2 = '_ES2',
  RED_FCC__VTB_420P10 = 'V010',
  // undefine
  RED_FCC_UNDF = 'UNDF'
};

enum FrameQueueSize {
  VIDEO_PICTURE_QUEUE_SIZE_MIN = 3,
  VIDEO_PICTURE_QUEUE_SIZE_MAX = 16,
  VIDEO_PICTURE_QUEUE_SIZE_DEFAULT = 3,
  SUBPICTURE_QUEUE_SIZE = 16,
  SAMPLE_QUEUE_SIZE = 9,
  FRAME_QUEUE_SIZE = 16
};

enum BufferWaterMark {
  DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS = 500,
  DEFAULT_NEXT_HIGH_WATER_MARK_IN_MS = 1000,
  DEFAULT_LAST_HIGH_WATER_MARK_IN_MS = 5000,
  DEFAULT_HIGH_WATER_MARK_IN_BYTES = 256 * 1024
};

enum CacheDuration {
  DEFAULT_MIN_CACHE_DURATION_IN_MS = 0,
  DEFAULT_MAX_CACHE_DURATION_IN_MS = 3600000
};

enum LogLevel {
  RED_LOG_UNKNOWN = 0,
  RED_LOG_DEFAULT = 1,
  RED_LOG_VERBOSE = 2,
  RED_LOG_DEBUG = 3,
  RED_LOG_INFO = 4,
  RED_LOG_WARN = 5,
  RED_LOG_ERROR = 6,
  RED_LOG_FATAL = 7,
  RED_LOG_SILENT = 8
};

struct TrackInfo {
  int stream_index{-1};
  int stream_type{-1};
  int codec_id{0};
  int codec_profile{0};
  int codec_level{0};
  int64_t bit_rate{0};

  /* Video */
  int width{0};
  int height{0};
  int sar_num{0};
  int sar_den{1};
  int fps_num{0};
  int fps_den{1};
  int tbr_num{0};
  int tbr_den{1};
  int rotation{0};
  int pixel_format{-1};
  uint8_t color_primaries{0};
  uint8_t color_trc{0};
  uint8_t color_space{0};
  uint8_t color_range{0};
  uint8_t skip_loop_filter{0};
  uint8_t skip_idct{0};
  uint8_t skip_frame{0};

  /* Audio */
  int sample_rate{0};
  int sample_fmt{0};
  uint64_t channel_layout{0};
  int channels{0};

  uint8_t *extra_data{nullptr};
  int extra_data_size{0};
  int time_base_num{0};
  int time_base_den{1};
  bool current_stream{false};
};

struct MetaData {
  ~MetaData() {
    for (auto iter = track_info.begin(); iter != track_info.end(); ++iter) {
      if (iter->extra_data) {
        delete[] iter->extra_data;
        iter->extra_data = nullptr;
      }
    }
  }
  std::string format_type;
  int64_t duration{0};
  int64_t start_time{0};
  int64_t bit_rate{0};
  int audio_index{-1};
  int video_index{-1};
  int sub_index{-1};
  std::vector<TrackInfo> track_info;
};
