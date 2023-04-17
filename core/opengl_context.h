#pragma once

#include <GL/glew.h>

struct OpenGLState {
  GLuint current_bind_program = 0;
};

class OpenGLContext final {
 public:
  OpenGLContext() = default;
  ~OpenGLContext() {}

  OpenGLContext(const OpenGLContext&) = delete;
  OpenGLContext& operator=(const OpenGLContext&) = delete;

  void set_cache_state(bool cache_state) {
    cache_state_ = cache_state;
  }

  bool cache_state() const {
    return cache_state_;
  }

  void glUseProgram(GLuint program) {
    if (!cache_state_ || program != states_.current_bind_program) {
      ::glUseProgram(program);
      states_.current_bind_program = program;
    }
  }
 private:
  OpenGLState states_;
  bool cache_state_ = true;
};