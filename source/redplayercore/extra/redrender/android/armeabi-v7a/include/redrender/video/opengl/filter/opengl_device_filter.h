/*
 * opengl_device_filter.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef OPENGL_DEVICE_FILTER_H
#define OPENGL_DEVICE_FILTER_H

#include "../../video_inc_internal.h"
#include "opengl_filter_base.h"

NS_REDRENDER_BEGIN

// opengl device filter class
class OpenGLDeviceFilter : public OpenGLFilterBase {
public:
  OpenGLDeviceFilter(const int &sessionID = 0);
  virtual ~OpenGLDeviceFilter() override;

  static std::shared_ptr<OpenGLFilterBase>
  create(VideoFrameMetaData *inputFrameMetaData, const int &sessionID = 0,
         std::shared_ptr<RedRender::Context> instance = nullptr);
  virtual VRError
  init(VideoFrameMetaData *inputFrameMetaData = nullptr,
       std::shared_ptr<RedRender::Context> instance = nullptr) override;
  virtual VRError onRender() override;
  virtual std::string getOpenGLFilterBaseName() override;
  virtual VideoFilterType getOpenGLFilterBaseType() override;

protected:
};

NS_REDRENDER_END

#endif /* OPENGL_DEVICE_FILTER_H */
