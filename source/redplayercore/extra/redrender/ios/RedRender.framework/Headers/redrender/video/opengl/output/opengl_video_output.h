/*
 * opengl_video_output.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef OPENGL_VIDEO_OUTPUT_H
#define OPENGL_VIDEO_OUTPUT_H

#include "../../video_inc_internal.h"
#include "../common/framebuffer.h"
#include "../input/opengl_video_input.h"
#include <map>

NS_REDRENDER_BEGIN

class OpenGLVideoOutput {
public:
  OpenGLVideoOutput(const int &sessionID = 0);
  virtual ~OpenGLVideoOutput();
  virtual std::shared_ptr<OpenGLVideoOutput>
  addTarget(const std::shared_ptr<OpenGLVideoInput> &target);
  virtual std::shared_ptr<OpenGLVideoOutput>
  addTarget(const std::shared_ptr<OpenGLVideoInput> &target, int textureIndex);
  virtual bool hasTarget(const std::shared_ptr<OpenGLVideoInput> &target) const;
  virtual void removeTarget(const std::shared_ptr<OpenGLVideoInput> &target);
  virtual void removeAllTargets();
  virtual void updateTargets(float frameTime);
  virtual void
  setFramebuffer(const std::shared_ptr<Framebuffer> &framebuffer,
                 RotationMode outputRotation = RotationMode::NoRotation);
  virtual std::shared_ptr<Framebuffer> getFramebuffer() const;
  virtual void releaseFramebuffer();

  void setFramebufferScale(float framebufferScale) {
    _framebufferScale = framebufferScale;
  }
  int getRotatedFramebufferWidth() const;
  int getRotatedFramebufferHeight() const;
  void setOutputRotation(RotationMode outputRotation);

protected:
  std::shared_ptr<Framebuffer> _framebuffer{
      nullptr}; /* Store the filter processing results */
  RotationMode _outputRotation;
  std::map<std::shared_ptr<OpenGLVideoInput>, int /* textureIndex */> _targets;
  float _framebufferScale;
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* OPENGL_VIDEO_OUTPUT_H */
