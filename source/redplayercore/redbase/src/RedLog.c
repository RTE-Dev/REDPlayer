#include "RedLog.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static LogContext kLogContext = {0};

#define TIME_BUF_SIZE (32)
#define PRINT_BUF_SIZE (1024)
#define OUT_BUF_SIZE (1024 + 256)

static void GetLocalTimeString(char *buf, int buf_size) {
  struct timeval t;
  gettimeofday(&t, NULL);
  struct tm *ptm = localtime(&t.tv_sec);
  snprintf(buf, buf_size, "%02d-%02d %02d:%02d:%02d.%03d", ptm->tm_mon + 1,
           ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
           (int)(t.tv_usec / 1000));
  return;
}

static char GetLogLevelChar(int lev) {
  switch (lev) {
  case AV_LEVEL_INFO:
    return 'I';
  case AV_LEVEL_DEBUG:
    return 'D';
  case AV_LEVEL_WARNING:
    return 'W';
  case AV_LEVEL_ERROR:
    return 'E';
  case AV_LEVEL_FATAL:
    return 'F';
  case AV_LEVEL_VERBOSE:
    return 'V';
  default:
    return ' ';
  }
  return ' ';
}

static void FormatLog(int level, const char *tag, int id, char *input_buf,
                      char *out_buf, int out_buf_size) {
  char time_buf[TIME_BUF_SIZE] = {0};
  char lev_c;
#if defined(__APPLE__)
  uint64_t atid;
  pthread_threadid_np(NULL, &atid);
#endif
  int pid = getpid();
  GetLocalTimeString(time_buf, TIME_BUF_SIZE);
  lev_c = GetLogLevelChar(level);
  if (id > 0) {
#if defined(__APPLE__)
    snprintf(out_buf, out_buf_size,
             "%s %d 0x%" PRIu64 " %c [%s]: [id @ %04d] %s", time_buf, pid, atid,
             lev_c, tag, id, input_buf);
#else
    snprintf(out_buf, out_buf_size, "[%s]: [id @ %04d] %s", tag, id, input_buf);
#endif
  } else {
#if defined(__APPLE__)
    snprintf(out_buf, out_buf_size, "%s %d 0x%" PRIu64 " %c [%s]: %s", time_buf,
             pid, atid, lev_c, tag, input_buf);
#else
    snprintf(out_buf, out_buf_size, "[%s]: %s", tag, input_buf);
#endif
  }

  return;
}

int RedLogPrint(int level, const char *tag, const char *fmt, ...) {
  if (kLogContext.level < level) {
    return 0;
  }

  char print_buf[PRINT_BUF_SIZE] = {0};
  char out_buf[OUT_BUF_SIZE] = {0};
  va_list args;
  int printed;
  va_start(args, fmt);
  printed = vsnprintf(print_buf, PRINT_BUF_SIZE - 1, fmt, args);
  va_end(args);

  if (printed <= 0) {
    return printed;
  }

  FormatLog(level, tag, 0, print_buf, out_buf, OUT_BUF_SIZE);
  if (kLogContext.callback) {
    kLogContext.callback(kLogContext.userdata, level, out_buf);
    return 0;
  }

  return 0;
}

int RedLogPrintId(int level, const char *tag, int id, const char *fmt, ...) {
  if (kLogContext.level < level) {
    return 0;
  }
  char print_buf[PRINT_BUF_SIZE] = {0};
  char out_buf[OUT_BUF_SIZE] = {0};
  va_list args;
  int printed;
  va_start(args, fmt);
  printed = vsnprintf(print_buf, PRINT_BUF_SIZE - 1, fmt, args);
  va_end(args);

  if (printed <= 0) {
    return printed;
  }

  FormatLog(level, tag, id, print_buf, out_buf, OUT_BUF_SIZE);
  if (kLogContext.callback) {
    kLogContext.callback(kLogContext.userdata, level, out_buf);
  }

  return 0;
}

void RedLogSetLevel(int level) { kLogContext.level = level; }

void RedLogSetCallback(void (*callback)(void *, int, const char *),
                       void *userdata) {
  kLogContext.callback = callback;
  kLogContext.userdata = userdata;
}
