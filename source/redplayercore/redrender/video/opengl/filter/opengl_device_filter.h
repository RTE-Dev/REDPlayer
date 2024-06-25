#pragma once

#include "../../video_inc_internal.h"
#include "./opengl_filter_base.h"

NS_REDRENDER_BEGIN

// opengl device filter class
class OpenGLDeviceFilter : public OpenGLFilterBase {
public:
  OpenGLDeviceFilter(const int &sessionID);
  ~OpenGLDeviceFilter() override;

  static std::shared_ptr<OpenGLFilterBase>
  create(VideoFrameMetaData *inputFrameMetaData, const int &sessionID,
         std::shared_ptr<RedRender::Context> instance);
  VRError init(VideoFrameMetaData *inputFrameMetaData,
               std::shared_ptr<RedRender::Context> instance) override;
  VRError onRender() override;
  std::string getOpenGLFilterBaseName() override;
  VideoFilterType getOpenGLFilterBaseType() override;

protected:
};

NS_REDRENDER_END
