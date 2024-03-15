#pragma once

#if defined(__ANDROID__)

#include "jni/Util.h"

#define JNI_CHECK_GOTO(condition__, env__, exception__, msg__, label__)        \
  do {                                                                         \
    if (!(condition__)) {                                                      \
      goto label__;                                                            \
    }                                                                          \
  } while (0)

#define JNI_CHECK_RET_VOID(condition__, env__, exception__, msg__)             \
  do {                                                                         \
    if (!(condition__)) {                                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define JNI_CHECK_RET(condition__, env__, exception__, msg__, ret__)           \
  do {                                                                         \
    if (!(condition__)) {                                                      \
      return ret__;                                                            \
    }                                                                          \
  } while (0)

#endif // __ANDROID__
