/*
 * framebuffer_cache.h
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */

#ifndef FRAMEBUFFER_CACHE_H
#define FRAMEBUFFER_CACHE_H

#include "../../video_inc_internal.h"
#include "framebuffer.h"
#include "gl_util.h"
#include <map>
#include <memory>
#include <string>

NS_REDRENDER_BEGIN

// opengl framebuffer cache class
class FramebufferCache {
public:
  FramebufferCache(const int &sessionID = 0);
  ~FramebufferCache();
  std::shared_ptr<Framebuffer> fetchFramebuffer();
  std::shared_ptr<Framebuffer>
  fetchFramebuffer(int width, int height, bool onlyTexture = false,
                   const TextureAttributes textureAttributes =
                       Framebuffer::defaultTextureAttribures);
  void returnFramebuffer(std::shared_ptr<Framebuffer> framebuffer,
                         bool isCache = false);
  void framebufferCacheClear();

private:
  std::string _getHash(int width, int height, bool onlyTexture,
                       const TextureAttributes textureAttributes) const;
  std::shared_ptr<Framebuffer> _getFramebufferByHash(const std::string &hash);

  std::map<std::string, std::shared_ptr<Framebuffer>> _framebuffers;
  std::map<std::string, int> _framebufferTypeCounts; //相同属性Framebuffer个数
  const int _sessionID{0};
};

NS_REDRENDER_END

#endif /* FRAMEBUFFER_CACHE_H */
