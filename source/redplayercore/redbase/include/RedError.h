/*
 * RedError.h
 *
 *  Created on: 2022年3月29日
 *      Author: liuhongda
 */

#pragma once

#include "RedBase.h"
#include <stdint.h>
#include <string>

typedef int32_t status_t;
typedef int32_t RED_ERR;

/*
 * Error codes.
 * All error codes are negative values.
 */

enum {
  OK = 0, // Preferred constant for checking success.
  ME_CLOSED,
#ifndef NO_ERROR
  // Win32 #defines NO_ERROR as well.  It has the same value, so there's no
  // real conflict, though it's a bit awkward.
  NO_ERROR = OK, // Deprecated synonym for `OK`. Prefer `OK` because it doesn't
                 // conflict with Windows.
#endif

  ME_ERROR = -1,
  ME_STATE_ERROR = -2,
  ME_RETRY = -3,

  UNKNOWN_ERROR = (-2147483647 - 1), // INT32_MIN value

  NO_MEMORY = -ENOMEM,
  INVALID_OPERATION = -ENOSYS,
  BAD_VALUE = -EINVAL,
  BAD_TYPE = (UNKNOWN_ERROR + 1),
  NAME_NOT_FOUND = -ENOENT,
  PERMISSION_DENIED = -EPERM,
  NO_INIT = -ENODEV,
  ALREADY_EXISTS = -EEXIST,
  DEAD_OBJECT = -EPIPE,
  FAILED_TRANSACTION = (UNKNOWN_ERROR + 2),
#if !defined(_WIN32)
  BAD_INDEX = -EOVERFLOW,
  NOT_ENOUGH_DATA = -ENODATA,
  WOULD_BLOCK = -EWOULDBLOCK,
  TIMED_OUT = -ETIMEDOUT,
  UNKNOWN_TRANSACTION = -EBADMSG,
#else
  BAD_INDEX = -E2BIG,
  NOT_ENOUGH_DATA = (UNKNOWN_ERROR + 3),
  WOULD_BLOCK = (UNKNOWN_ERROR + 4),
  TIMED_OUT = (UNKNOWN_ERROR + 5),
  UNKNOWN_TRANSACTION = (UNKNOWN_ERROR + 6),
#endif
  FDS_NOT_ALLOWED = (UNKNOWN_ERROR + 7),
  UNEXPECTED_NULL = (UNKNOWN_ERROR + 8),
};

enum {
  ERROR_UNKOWN = 0,           // unkown error
  ERROR_INIT = 1,             // init
  ERROR_REDPLAYER_CREATE = 2, // create errorcode :AVERROR(ENOMEM)
  ERROR_OPTION = 10,          // setoption  errorcode :AVERROR(ENOMEM)
  ERROR_PREPARE = 20,         // prepare
  ERROR_STREAM_OPEN,          // stream open //AVERROR(ENOMEM),SDL_ERROR
  ERROR_FORMAT_ALLOC,         // format alloc
  ERROR_OPEN_AOUT,            // create audio decoder // deprecate
  ERROR_OPEN_VOUT,            // create video decoder // deprecate
  ERROR_OPEN_INPUT,           // PROTOCOL, DEMUX
  ERROR_OPEN_INPUT_RECONNECT,
  ERROR_OPEN_INPUT_SWITCHCDN,
  ERROR_OPEN_INPUT_CACHE,
  ERROR_FIND_STREAM_INFO,    // find stream info //ERROR CODE
  ERROR_COMPONENT_OPEN = 30, // init decoder //AVERROR(EINVAL);AVERROR(ENOMEM)

  ERROR_READ_FRAME, // read frame
  ERROR_READ_FRAME_RECONNECT = 32,

  ERROR_DECODER = 40,     // Other decoder errors
  ERROR_DECODER_VDEC,     // video decoder
  ERROR_DECODER_ADEC,     // audio decoder
  ERROR_DECODER_SUBTITLE, // subtitle decoder

  ERROR_VIDEO_DISPLAY,      // Other video display errors
  ERROR_VIDEO_DISPLAY_VIEW, // video display view error
  ERROR_AUDIO_DISPLAY,      // Other audio displays

  ERROR_FFMPEG, // Other ffmpeg errors

  ERROR_NETWORK,   // Other network errors
  ERROR_TCP,       // tcp error
  ERROR_HTTP = 50, // http error

  ERROR_ANDROID_SYSTEM,        // Other android system errors
  ERROR_IOS_SYSTEM,            // Other ios system errors
  ERROR_VTB_VDEC,              // ios-vtb
  ERROR_VTB_FALLBACK,          // ios-vtb // temporarily useless
  ERROR_REDDECODER_ERROR = 55, // reddecoder errors // deprecate

  ERROR_DNS, // dns error

  // this error will trigger player fallback
  ERROR_INVALID_DATA, // decrypted data ctts invalid errors
};

enum {
  DECODER_FALLBACK = 1000,
  DECODER_INIT_ERROR = 1001,
  DECODER_NO_OUTPUT = 1002,
  DECODER_CALLBACK_ERROR_BASE = 2000,
  DECODER_FORMAT_ERROR_BASE = 3000,
} DecoderError;

// Human readable name of error
inline std::string StatusToString(status_t status) {
#define STATUS_CASE(STATUS)                                                    \
  case STATUS:                                                                 \
    return #STATUS

  switch (status) {
    STATUS_CASE(OK);
    STATUS_CASE(UNKNOWN_ERROR);
    STATUS_CASE(NO_MEMORY);
    STATUS_CASE(INVALID_OPERATION);
    STATUS_CASE(BAD_VALUE);
    STATUS_CASE(BAD_TYPE);
    STATUS_CASE(NAME_NOT_FOUND);
    STATUS_CASE(PERMISSION_DENIED);
    STATUS_CASE(NO_INIT);
    STATUS_CASE(ALREADY_EXISTS);
    STATUS_CASE(DEAD_OBJECT);
    STATUS_CASE(FAILED_TRANSACTION);
    STATUS_CASE(BAD_INDEX);
    STATUS_CASE(NOT_ENOUGH_DATA);
    STATUS_CASE(WOULD_BLOCK);
    STATUS_CASE(TIMED_OUT);
    STATUS_CASE(UNKNOWN_TRANSACTION);
    STATUS_CASE(FDS_NOT_ALLOWED);
    STATUS_CASE(UNEXPECTED_NULL);
#undef STATUS_CASE
  }

  return std::to_string(status) + " (" + strerror(-status) + ")";
}
