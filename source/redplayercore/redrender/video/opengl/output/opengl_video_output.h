#pragma once

#include <map>

#include "../../video_inc_internal.h"
#include "../common/framebuffer.h"
#include "../input/opengl_video_input.h"

NS_REDRENDER_BEGIN

class OpenGLVideoOutput {
public:
  OpenGLVideoOutput(const int &sessionID = 0);
  virtual ~OpenGLVideoOutput();

protected:
  std::shared_ptr<Framebuffer> _framebuffer{
      nullptr}; /* Store the filter processing results */
  float _framebufferScale;

private:
  const int _sessionID{0};
};

NS_REDRENDER_END
