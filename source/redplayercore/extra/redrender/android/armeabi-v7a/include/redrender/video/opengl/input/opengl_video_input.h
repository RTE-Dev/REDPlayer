/*
 * opengl_video_input.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef OPENGL_VIDEO_INPUT_H
#define OPENGL_VIDEO_INPUT_H

#include "../../video_inc_internal.h"
#include "../common/framebuffer.h"
#include <map>

NS_REDRENDER_BEGIN

class OpenGLVideoInput {
public:
  OpenGLVideoInput(int inputNum = 1, const int &sessionID = 0);
  virtual ~OpenGLVideoInput();
  virtual void
  setInputFramebuffer(const std::shared_ptr<Framebuffer> &framebuffer,
                      RotationMode rotationMode = RotationMode::NoRotation,
                      int textureIndex = 0);
  virtual bool isPrepared() const;
  virtual void unPrepare();
  virtual int getNextAvailableTextureIndex() const;
  virtual void clearInputFramebuffer();
  void setInputTexNum(int inputTexNum);

protected:
  struct InputFrameBufferInfo {
    std::shared_ptr<Framebuffer> frameBuffer;
    RotationMode rotationMode;
    int textureIndex;
    bool ignoreForPrepare;
  };

  std::map<int /* textureIndex */, InputFrameBufferInfo> _inputFramebuffers;
  int _inputTexNum;
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* OPENGL_VIDEO_INPUT_H */
