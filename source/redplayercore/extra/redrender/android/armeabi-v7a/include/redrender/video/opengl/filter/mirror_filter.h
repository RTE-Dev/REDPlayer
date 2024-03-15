/*
 * mirror_filter.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef MIRROR_FILTER_H
#define MIRROR_FILTER_H

#include "../../video_inc_internal.h"
#include "opengl_filter_base.h"

NS_REDRENDER_BEGIN

typedef enum MirrorType {
  MirrorTypeWholeImage = 0,
  MirrorTypeLeftAndRight,
  MirrorTypeUpAndDown,
} MirrorType;

// opengl mirror filter class
class MirrorFilter : public OpenGLFilterBase {
public:
  MirrorFilter(const int &sessionID = 0);
  virtual ~MirrorFilter() override;

  static std::shared_ptr<OpenGLFilterBase>
  create(const int &sessionID = 0,
         std::shared_ptr<RedRender::Context> instance = nullptr);
  virtual VRError
  init(VideoFrameMetaData *inputFrameMetaData = nullptr,
       std::shared_ptr<RedRender::Context> instance = nullptr) override;
  virtual VRError onRender() override;
  virtual std::string getOpenGLFilterBaseName() override;
  virtual VideoFilterType getOpenGLFilterBaseType() override;
  void setMirrorType(MirrorType mirrorType);

protected:
private:
  int _mirrorType; // default _mirrorType=MirrorTypeWholeImage
};

NS_REDRENDER_END

#endif /* MIRROR_FILTER_H */
