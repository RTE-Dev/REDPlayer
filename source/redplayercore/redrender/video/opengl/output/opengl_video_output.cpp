/*
 * opengl_video_output.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./opengl_video_output.h"

NS_REDRENDER_BEGIN

OpenGLVideoOutput::OpenGLVideoOutput(const int &sessionID)
    : _framebuffer(nullptr), _framebufferScale(1.0), _sessionID(sessionID) {
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

OpenGLVideoOutput::~OpenGLVideoOutput() {
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

NS_REDRENDER_END
