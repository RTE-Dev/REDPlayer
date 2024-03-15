/*
 * yuv2rgb_filter.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef YUV2RGB_FILTER_H
#define YUV2RGB_FILTER_H

#include "../../video_inc_internal.h"
#include "opengl_filter_base.h"

NS_REDRENDER_BEGIN

// opengl filter yuv2rgb class
class YUV2RGBFilter : public OpenGLFilterBase {
public:
  YUV2RGBFilter(const int &sessionID = 0);
  virtual ~YUV2RGBFilter() override;

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
  VRPixelFormat _videoRenderPixelFormat;
  VRAVColorSpace _videoRenderAVColorSpace;
  VRAVColorRange _videoRenderAVColorRange;
};

NS_REDRENDER_END

#endif /* YUV2RGB_FILTER_H */
