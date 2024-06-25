/*
 * gl_util.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./gl_util.h"

NS_REDRENDER_BEGIN

std::string strFormat(const char *fmt, ...) {
  std::string strResult = "";
  if (NULL != fmt) {
    va_list marker;
    va_start(marker, fmt);
    char *buf = 0;
    int result = vasprintf(&buf, fmt, marker);
    if (!buf) {
      va_end(marker);
      return strResult;
    }

    if (result < 0) {
      free(buf);
      va_end(marker);
      return strResult;
    }

    result = static_cast<int>(strlen(buf));
    strResult.append(buf, result);
    free(buf);
    va_end(marker);
  }
  return strResult;
}

NS_REDRENDER_END
