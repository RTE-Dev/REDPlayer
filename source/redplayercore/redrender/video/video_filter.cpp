/*
 * video_filter.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./video_filter.h"

NS_REDRENDER_BEGIN

VideoFilter::VideoFilter(const int &sessionID) : _sessionID(sessionID) {
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

VideoFilter::~VideoFilter() {
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

NS_REDRENDER_END
