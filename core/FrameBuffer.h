#pragma once
#include <GL/glew.h>
#include "GLObject.h"
#include "Texture2D.h"
#include "GLObject.h"

class FrameBuffer : public GLObject {
  int w, h;

public:
  operator bool();
  FrameBuffer();
  ~FrameBuffer();

  void Create();
  void AttachColorTexture(Texture2D *texture, int attachmentId);
  void AttachDepthTexture(Texture2D *texture);
  void AttachDepthStencilTexture(Texture2D *texture);

  void DrawBuffers(GLenum *buffers, int count);

  bool isComplete();

  void Bind(GLenum framebuffer_bind_pos = GL_FRAMEBUFFER);
  void Unbind();

  void Destroy();

  int Width();
  int Height();
};
