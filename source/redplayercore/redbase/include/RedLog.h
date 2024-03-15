#pragma once

#include <stdint.h>

#define AV_LEVEL_NONE 0
#define AV_LEVEL_FATAL 8
#define AV_LEVEL_ERROR 16
#define AV_LEVEL_WARNING 24
#define AV_LEVEL_INFO 32
#define AV_LEVEL_DEBUG 48
#define AV_LEVEL_VERBOSE 56

#ifdef __cplusplus
extern "C" {
#endif
__attribute__((__format__(printf, 3, 4))) int
RedLogPrint(int level, const char *tag, const char *fmt, ...);
__attribute__((__format__(printf, 4, 5))) int
RedLogPrintId(int level, const char *tag, int id, const char *fmt, ...);
void RedLogSetLevel(int level);
void RedLogSetCallback(void (*callback)(void *, int, const char *),
                       void *userdata);
#ifdef __cplusplus
}
#endif

typedef struct LogContext {
  int level;
  void *userdata;
  void (*callback)(void *arg, int level, const char *buf);
} LogContext;

#ifndef NOLOGI
#define AV_LOGI(...) RedLogPrint(AV_LEVEL_INFO, __VA_ARGS__)
#else
#define AV_LOGI(...)
#endif
#ifndef NOLOGD
#define AV_LOGD(...) RedLogPrint(AV_LEVEL_DEBUG, __VA_ARGS__)
#else
#define AV_LOGD(...)
#endif
#ifndef NOLOGW
#define AV_LOGW(...) RedLogPrint(AV_LEVEL_WARNING, __VA_ARGS__)
#else
#define AV_LOGW(...)
#endif
#define AV_LOGE(...) RedLogPrint(AV_LEVEL_ERROR, __VA_ARGS__)

#ifndef NOLOGI
#define AV_LOGI_ID(...) RedLogPrintId(AV_LEVEL_INFO, __VA_ARGS__)
#else
#define AV_LOGI_ID(...)
#endif
#ifndef NOLOGD
#define AV_LOGD_ID(...) RedLogPrintId(AV_LEVEL_DEBUG, __VA_ARGS__)
#else
#define AV_LOGD_ID(...)
#endif
#ifndef NOLOGW
#define AV_LOGW_ID(...) RedLogPrintId(AV_LEVEL_WARNING, __VA_ARGS__)
#else
#define AV_LOGW_ID(...)
#endif
#define NOLOGV 1
#ifndef NOLOGV
#define AV_LOGV(...) RedLogPrint(AV_LEVEL_VERBOSE, __VA_ARGS__)
#else
#define AV_LOGV(...)
#endif
#ifndef NOLOGV
#define AV_LOGV_ID(...) RedLogPrintId(AV_LEVEL_VERBOSE, __VA_ARGS__)
#else
#define AV_LOGV_ID(...)
#endif
#define AV_LOGE_ID(...) RedLogPrintId(AV_LEVEL_ERROR, __VA_ARGS__)

#ifndef AV_LOGF_IF
#define AV_LOGF_IF(cond, ...)                                                  \
  do {                                                                         \
    if ((cond)) {                                                              \
      RedLogPrint(AV_LEVEL_FATAL, __VA_ARGS__);                                \
      abort();                                                                 \
    }                                                                          \
  } while (false)
#endif

#ifndef AV_LOGF
#define AV_LOGF(...)                                                           \
  do {                                                                         \
    RedLogPrint(AV_LEVEL_FATAL, __VA_ARGS__);                                  \
    abort();                                                                   \
  } while (false)
#endif
