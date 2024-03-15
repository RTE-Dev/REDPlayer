/*
 * black_and_white_filter.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef BLACK_AND_WHITE_FILTER_H
#define BLACK_AND_WHITE_FILTER_H

#include "../../video_inc_internal.h"
#include "opengl_filter_base.h"

NS_REDRENDER_BEGIN

typedef enum BlackAndWhiteType {
  BlackAndWhiteTypeYUV = 0,
  BlackAndWhiteTypeAverage,
} BlackAndWhiteType;

// opengl black and white filter class
class BlackAndWhiteFilter : public OpenGLFilterBase {
public:
  BlackAndWhiteFilter(const int &sessionID = 0);
  virtual ~BlackAndWhiteFilter() override;

  static std::shared_ptr<OpenGLFilterBase>
  create(const int &sessionID = 0,
         std::shared_ptr<RedRender::Context> instance = nullptr);
  virtual VRError
  init(VideoFrameMetaData *inputFrameMetaData = nullptr,
       std::shared_ptr<RedRender::Context> instance = nullptr) override;
  virtual VRError onRender() override;
  virtual std::string getOpenGLFilterBaseName() override;
  virtual VideoFilterType getOpenGLFilterBaseType() override;
  void setBlackAndWhiteType(BlackAndWhiteType blackAndWhiteType);

protected:
private:
  int _blackAndWhiteType;
};

NS_REDRENDER_END

#endif /* BLACK_AND_WHITE_FILTER_H */
