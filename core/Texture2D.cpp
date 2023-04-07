#include "Texture2D.h"
#include "FrameBuffer.h"
#include <memory>
#include "core/utility.h"

namespace {
FrameBuffer readTexFBO;
}

Texture2D::operator bool() { return id; }

Texture2D::Texture2D() {}

Texture2D::~Texture2D() { Destroy(); }

void Texture2D::Create(int levels, int w, int h, GLenum internalFormat,
                       GLenum format, GLenum type, void *data) {
  this->w = w;
  this->h = h;
  this->levels = levels < 1 ? 1 : levels;
  this->internalFormat = internalFormat;
  glGenTextures(1, &id);
  Bind(0);

  glTexStorage2D(GL_TEXTURE_2D, levels, this->internalFormat, this->w, this->h);
  if (data) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, data);
  }

  // glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->w, this->h, 0,
  // format, type, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::SetTextureFilter(GLenum magfilter, GLenum minFilter) {
  Bind(0);
  minFilter = minFilter;
  magFilter = magfilter;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
}

void Texture2D::SetTextureWrapMod(GLenum wrapS_, GLenum wrapT_) {
  Bind(0);
  wrapS = wrapS_;
  wrapT = wrapT_;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}

void Texture2D::SetBorderColor(float *color) {
  Bind(0);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
}

void Texture2D::Resize(int w, int h) {
  Bind(0);
  this->w = w;
  this->h = h;
  Destroy();
  Create(levels, this->w, this->h, internalFormat, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}

void Texture2D::FillData(int level, int x, int y, int w, int h, GLenum format,
                         GLenum type, void *data) {
  Bind(0);
  glTexSubImage2D(GL_TEXTURE_2D, level, x, y, w, h, format, type, data);
}

void Texture2D::Bind(int location) {
  glActiveTexture(GL_TEXTURE0 + location);
  glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2D::ReadData(void *buffer, int x, int y, int w, int h,
                         GLenum format, GLenum type) {
  FramebufferGuard framebuffer_g;

  if (!readTexFBO.ID()) {
    readTexFBO.Create();
  }
  readTexFBO.AttachColorTexture(this, 0);
  readTexFBO.Bind();
  glReadPixels(x, y, w, h, format, type, buffer);
  readTexFBO.Unbind();
}

void Texture2D::ReadScreenData(void *buffer, int x, int y, int w, int h,
                               GLenum format, GLenum type) {
  FramebufferGuard framebuffer_g;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadPixels(x, y, w, h, format, type, buffer);
}

void Texture2D::Unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

void Texture2D::Destroy() {
  if (id) {
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &id);
    id = 0;
  }
}

int Texture2D::Width() { return w; }

int Texture2D::Height() { return h; }

int Texture2D::Levels() { return levels; }

GLenum Texture2D::InternalFormat() { return internalFormat; }

Texture2D Texture2D::Clone() {
  Texture2D tex;
  tex.Create(levels, w, h, tex.internalFormat, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  tex.Bind(0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);
  return tex;
}