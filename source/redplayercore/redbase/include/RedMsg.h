/*
 * RedMsg.h
 *
 *  Created on: 2022年9月9日
 *      Author: liuhongda
 */

#pragma once

enum PlayerReq {
  RED_REQ_START = 20001,
  RED_REQ_PAUSE = 20002,
  RED_REQ_SEEK = 20003,
  RED_REQ_INTERNAL_PAUSE = 21001,
  RED_REQ_INTERNAL_PLAYBACK_RATE = 21002
};

enum PlayerMsg {
  RED_MSG_FLUSH = 0,

  RED_MSG_BPREPARED = 2,

  RED_MSG_ERROR = 100, /* arg1 = error */

  RED_MSG_PREPARED = 200,

  RED_MSG_COMPLETED = 300,

  RED_MSG_VIDEO_SIZE_CHANGED = 400, /* arg1 = width, arg2 = height */
  RED_MSG_SAR_CHANGED = 401,        /* arg1 = sar.num, arg2 = sar.den */
  RED_MSG_VIDEO_RENDERING_START = 402,
  RED_MSG_AUDIO_RENDERING_START = 403,
  RED_MSG_VIDEO_ROTATION_CHANGED = 404, /* arg1 = degree */
  RED_MSG_AUDIO_DECODED_START = 405,
  RED_MSG_VIDEO_DECODED_START = 406,
  RED_MSG_OPEN_INPUT = 407,
  RED_MSG_FIND_STREAM_INFO = 408,
  RED_MSG_COMPONENT_OPEN = 409,
  RED_MSG_VIDEO_SEEK_RENDERING_START = 410,
  RED_MSG_AUDIO_SEEK_RENDERING_START = 411,
  RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER = 412,
  RED_MSG_VIDEO_START_ON_PLAYING =
      413, /* 视频启播，计算节点为视频第二帧开始渲染 */

  RED_MSG_BUFFERING_START = 500,
  RED_MSG_BUFFERING_END = 501,
  RED_MSG_BUFFERING_UPDATE = 502, /* arg1 = buffering head position in time,
                                     arg2 = minimum percent in time or bytes */
  RED_MSG_BUFFERING_BYTES_UPDATE =
      503, /* arg1 = cached data in bytes,            arg2 = high water mark */
  RED_MSG_BUFFERING_TIME_UPDATE =
      504, /* arg1 = cached duration in milliseconds, arg2 = high water mark */

  RED_MSG_SEEK_COMPLETE =
      600, /* arg1 = seek position,                   arg2 = error */

  RED_MSG_PLAYBACK_STATE_CHANGED = 700,

  RED_MSG_TIMED_TEXT = 800,

  RED_MSG_ACCURATE_SEEK_COMPLETE = 900, /* arg1 = current position*/
  RED_MSG_SEEK_LOOP_START = 901,        /* arg1 = loop count */
  RED_MSG_URL_CHANGE = 902,

  RED_MSG_VIDEO_DECODER_OPEN = 10001,
  RED_MSG_VTB_COLOR_PRIMARIES_SMPTE432 = 10002
};
