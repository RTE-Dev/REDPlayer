/*
 * opengl_video_input.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./opengl_video_input.h"

NS_REDRENDER_BEGIN

OpenGLVideoInput::OpenGLVideoInput(int inputTexNum, const int &sessionID)
    : _inputTexNum(inputTexNum), _sessionID(sessionID) {
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
}

OpenGLVideoInput::~OpenGLVideoInput() {
  AV_LOGD_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  clearInputFramebuffer();
}

void OpenGLVideoInput::setInputFramebuffer(
    const std::shared_ptr<Framebuffer> &framebuffer, RotationMode rotationMode,
    int textureIndex) {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  InputFrameBufferInfo inputFrameBufferInfo;
  inputFrameBufferInfo.frameBuffer = framebuffer;
  inputFrameBufferInfo.rotationMode = rotationMode;
  inputFrameBufferInfo.textureIndex = textureIndex;
  std::map<int /* textureIndex */, InputFrameBufferInfo>::iterator iter =
      _inputFramebuffers.find(textureIndex);
  if (iter != _inputFramebuffers.end() && iter->second.frameBuffer) {
    _inputFramebuffers.erase(iter);
  }
  _inputFramebuffers[textureIndex] = inputFrameBufferInfo;
}

bool OpenGLVideoInput::isPrepared() const {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  int preparedNum = 0;
  for (std::map<int, InputFrameBufferInfo>::const_iterator iter =
           _inputFramebuffers.begin();
       iter != _inputFramebuffers.end(); ++iter) {
    if (iter->second.frameBuffer)
      preparedNum++;
  }
  if (preparedNum == _inputTexNum)
    return true;
  else
    return false;
}

void OpenGLVideoInput::clearInputFramebuffer() {
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  for (auto iter = _inputFramebuffers.begin();
       iter != _inputFramebuffers.end();) {
    if (iter->second.frameBuffer) {
      _inputFramebuffers.erase(iter++);
    }
  }
  _inputFramebuffers.clear();
}

NS_REDRENDER_END
