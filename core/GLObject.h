#pragma once

#include <GL/gl.h>

class GLObject
{
  GLObject(const GLObject &) = delete;
  GLObject &operator=(const GLObject &) = delete;

protected:
  GLuint id = 0;

public:
  GLObject() = default;
  virtual ~GLObject() = default;

  GLObject(GLObject &&other) : id(other.id)
  {
    other.id = 0;
  }
  GLObject &operator=(GLObject &&other)
  {
    id = other.id;
    other.id = 0;
    return *this;
  }

  GLuint ID() { return id; }
};