#pragma once

namespace reddecoder {
enum class AudioCodecError {
  kNoError = 0,
  kInvalidParameter = 1,
  kInitError = 2,
  kUninited = 3,
  kInternalError = 4,
  kEOF = 5,
};

enum class AudioSampleFormat {
  kUnknow = 0,
  kPCMS16 = 1,
};
}  // namespace reddecoder
