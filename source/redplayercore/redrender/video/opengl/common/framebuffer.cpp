/*
 * framebuffer.cpp
 * RedRender
 *
 * Copyright (c) 2022 xiaohongshu. All rights reserved.
 * Created by shibu.
 *
 * This file is part of RedRender.
 */
#include "./framebuffer.h"

#include <string>

#include "./gl_util.h"

NS_REDRENDER_BEGIN

TextureAttributes Framebuffer::defaultTextureAttribures = {
    .minFilter = GL_LINEAR,
    .magFilter = GL_LINEAR,
    .wrapS = GL_CLAMP_TO_EDGE,
    .wrapT = GL_CLAMP_TO_EDGE,
    .internalFormat = GL_RGBA,
    .format = GL_RGBA,
    .type = GL_UNSIGNED_BYTE};

Framebuffer::Framebuffer(const int &sessionID /* = 0*/)
    : _texture(-1), _framebuffer(-1), _sessionID(sessionID) {
  _isTextureCache = true;
}

Framebuffer::Framebuffer(
    int width, int height, bool onlyGenerateTexture /* = false*/,
    const TextureAttributes textureAttributes /* = defaultTextureAttribures*/,
    const int &sessionID /* = 0 */)
    : _texture(-1), _framebuffer(-1), _sessionID(sessionID) {
  _width = width;
  _height = height;
  _textureAttributes = textureAttributes;
  _hasFramebuffer = !onlyGenerateTexture;

  std::string framebufferHash;
  if (_hasFramebuffer) {
    _generateFramebuffer();
    framebufferHash =
        strFormat("%.1dx%.1d-%d:%d:%d:%d:%d:%d:%d", _width, _height,
                  _textureAttributes.minFilter, _textureAttributes.magFilter,
                  _textureAttributes.wrapS, _textureAttributes.wrapT,
                  _textureAttributes.internalFormat, _textureAttributes.format,
                  _textureAttributes.type);
  } else {
    _generateTexture();
    framebufferHash =
        strFormat("%.1dx%.1d-%d:%d:%d:%d:%d:%d:%d-NOFB", _width, _height,
                  _textureAttributes.minFilter, _textureAttributes.magFilter,
                  _textureAttributes.wrapS, _textureAttributes.wrapT,
                  _textureAttributes.internalFormat, _textureAttributes.format,
                  _textureAttributes.type);
  }
  _isTextureCache = false;
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] create Framebuffer:%s .\n",
             __FUNCTION__, __LINE__, framebufferHash.c_str());
}

std::shared_ptr<Framebuffer>
Framebuffer::create(const int &sessionID /* = 0*/) {
  std::shared_ptr<Framebuffer> ptr = std::make_shared<Framebuffer>(sessionID);
  return ptr;
}

std::shared_ptr<Framebuffer> Framebuffer::create(
    int width, int height, bool onlyGenerateTexture /* = false*/,
    const TextureAttributes textureAttributes /* = defaultTextureAttribures*/,
    const int &sessionID /* = 0 */) {
  AV_LOGV_ID(LOG_TAG, sessionID, "[%s:%d] .\n", __FUNCTION__, __LINE__);
  std::shared_ptr<Framebuffer> ptr = std::make_shared<Framebuffer>(
      width, height, onlyGenerateTexture, textureAttributes, sessionID);

  return ptr;
}

Framebuffer::~Framebuffer() {
  if (!_isTextureCache) {
    release();
  }
}

void Framebuffer::release() {
  std::string framebufferHash;
  if (_hasFramebuffer) {
    framebufferHash =
        strFormat("%.1dx%.1d-%d:%d:%d:%d:%d:%d:%d", _width, _height,
                  _textureAttributes.minFilter, _textureAttributes.magFilter,
                  _textureAttributes.wrapS, _textureAttributes.wrapT,
                  _textureAttributes.internalFormat, _textureAttributes.format,
                  _textureAttributes.type);
  } else {
    framebufferHash =
        strFormat("%.1dx%.1d-%d:%d:%d:%d:%d:%d:%d-NOFB", _width, _height,
                  _textureAttributes.minFilter, _textureAttributes.magFilter,
                  _textureAttributes.wrapS, _textureAttributes.wrapT,
                  _textureAttributes.internalFormat, _textureAttributes.format,
                  _textureAttributes.type);
  }
  AV_LOGV_ID(LOG_TAG, _sessionID, "[%s:%d] release Framebuffer:%s .\n",
             __FUNCTION__, __LINE__, framebufferHash.c_str());
  if (-1 != _texture) {
    CHECK_OPENGL(glDeleteTextures(1, &_texture));
    _texture = -1;
  } else {
    AV_LOGW_ID(LOG_TAG, _sessionID, "[%s:%d] wrong texture.\n", __FUNCTION__,
               __LINE__);
  }
  if (_hasFramebuffer) {
    if (-1 != _framebuffer) {
      CHECK_OPENGL(glDeleteFramebuffers(1, &_framebuffer));
      _framebuffer = -1;
    } else {
      AV_LOGW_ID(LOG_TAG, _sessionID, "[%s:%d] wrong framebuffer.\n",
                 __FUNCTION__, __LINE__);
    }
  }
}

void Framebuffer::setViewPort() {
  CHECK_OPENGL(glViewport(0, 0, _width, _height));
}

void Framebuffer::activeFramebuffer() {
  CHECK_OPENGL(glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer));
}

void Framebuffer::inActiveFramebuffer() {
  CHECK_OPENGL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

#if REDRENDER_PLATFORM == REDRENDER_PLATFORM_IOS
void Framebuffer::setTexture(GLuint texture) { _texture = texture; }

void Framebuffer::setWidth(int width) { _width = width; }

void Framebuffer::setHeight(int height) { _height = height; }
#endif

void Framebuffer::_generateTexture() {
  CHECK_OPENGL(glGenTextures(1, &_texture));
  CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, _texture));
  CHECK_OPENGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                               _textureAttributes.minFilter));
  CHECK_OPENGL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                               _textureAttributes.magFilter));
  CHECK_OPENGL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                               _textureAttributes.wrapS));
  CHECK_OPENGL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                               _textureAttributes.wrapT));

  // TODO: Handle mipmaps
  CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
}

void Framebuffer::_generateFramebuffer() {
  CHECK_OPENGL(glGenFramebuffers(1, &_framebuffer));
  CHECK_OPENGL(glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer));
  _generateTexture();
  CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, _texture));
  CHECK_OPENGL(glTexImage2D(GL_TEXTURE_2D, 0, _textureAttributes.internalFormat,
                            _width, _height, 0, _textureAttributes.format,
                            _textureAttributes.type, 0));
  CHECK_OPENGL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, _texture, 0));
  CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
  CHECK_OPENGL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

NS_REDRENDER_END
