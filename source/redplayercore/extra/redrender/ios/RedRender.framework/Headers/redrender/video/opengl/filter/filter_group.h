/*
 * filter_group.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef FILTER_GROUP_H
#define FILTER_GROUP_H

#include "../../video_inc_internal.h"
#include "opengl_filter_base.h"

NS_REDRENDER_BEGIN

// todo
// opengl filter group class
class FilterGroup : public OpenGLFilterBase {
public:
  FilterGroup(const int &sessionID = 0);
  virtual ~FilterGroup() override;

  static std::shared_ptr<OpenGLFilterBase>
  create(const int &sessionID = 0,
         std::shared_ptr<RedRender::Context> instance = nullptr);
  virtual VRError
  init(VideoFrameMetaData *inputFrameMetaData = nullptr,
       std::shared_ptr<RedRender::Context> instance = nullptr) override;
  virtual VRError onRender() override;
  virtual void updateParam(float frameTime) override;
  virtual std::string getOpenGLFilterBaseName() override;
  virtual VideoFilterType getOpenGLFilterBaseType() override;

  virtual std::shared_ptr<OpenGLVideoOutput>
  addTarget(const std::shared_ptr<OpenGLVideoInput> &target) override;
  virtual void
  removeTarget(const std::shared_ptr<OpenGLVideoInput> &target) override;
  virtual void removeAllTargets() override;
  virtual void updateTargets(float frameTime) override;

  virtual void
  setInputFramebuffer(const std::shared_ptr<Framebuffer> &framebuffer,
                      RotationMode rotationMode = RotationMode::NoRotation,
                      int textureIndex = 0) override;

  bool init(std::vector<std::shared_ptr<OpenGLFilterBase>> &filters);
  bool hasFilter(const std::shared_ptr<OpenGLFilterBase> &filter) const;
  void addFilter(std::shared_ptr<OpenGLFilterBase> &filter);
  void removeFilter(std::shared_ptr<OpenGLFilterBase> &filter);
  void removeAllFilters();

  void setTerminalFilter(std::shared_ptr<OpenGLFilterBase> filter) {
    _terminalFilter = filter;
  }

protected:
  std::vector<std::shared_ptr<OpenGLFilterBase>> _filters;
  std::shared_ptr<OpenGLFilterBase> _terminalFilter;
};

NS_REDRENDER_END

#endif /* FILTER_GROUP_H */
