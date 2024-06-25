#include "redplayer_logcallback.h"
#include "hilog/log.h"
#include "redplayer_callarktslog.h"
#include "uv.h"

void redLogCallback(int level, const char *tag, const char *buffer) {
  std::string stringContent = buffer;
  std::string stringTag = tag;
  RedLog_callOnLogs(level, stringTag, stringContent);
}
