#pragma once

#include <map>

#include "../../video_inc_internal.h"
#include "../common/framebuffer.h"

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
  virtual void clearInputFramebuffer();

protected:
  struct InputFrameBufferInfo {
    std::shared_ptr<Framebuffer> frameBuffer;
    RotationMode rotationMode;
    int textureIndex;
  };

  std::map<int /* textureIndex */, InputFrameBufferInfo> _inputFramebuffers;
  int _inputTexNum;

private:
  const int _sessionID{0};
};

NS_REDRENDER_END
