#pragma once

#include <GL/glew.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class ShaderManager {
 public:
  ShaderManager() = default;
  ~ShaderManager() {
    for (auto& k_v : loaded_shaders_) {
      glDeleteShader(k_v.second);
    }
    for (auto& k_v : loaded_programs_) {
      glDeleteProgram(k_v.second);
    }
  }

  void LoadShaderForName(const std::string& shader_name,
                         const std::string& vert_src_path,
                         const std::string& frag_src_path,
                         bool reload = false) {
    if (loaded_shaders_.find(vert_src_path) == loaded_shaders_.end()) {
      loaded_shaders_[vert_src_path] =
          CompileShader(GL_VERTEX_SHADER, vert_src_path.c_str());
    } else {
      if (reload) {
        glDeleteShader(loaded_shaders_[vert_src_path]);
        loaded_shaders_[vert_src_path] =
            CompileShader(GL_VERTEX_SHADER, vert_src_path.c_str());
      }
    }
    if (loaded_shaders_.find(frag_src_path) == loaded_shaders_.end()) {
      loaded_shaders_[frag_src_path] =
          CompileShader(GL_FRAGMENT_SHADER, frag_src_path.c_str());
    } else {
      if (reload) {
        glDeleteShader(loaded_shaders_[frag_src_path]);
        loaded_shaders_[frag_src_path] =
            CompileShader(GL_FRAGMENT_SHADER, frag_src_path.c_str());
      }
    }

    if (loaded_programs_.find(shader_name) == loaded_programs_.end()) {
      loaded_programs_[shader_name] = LinkShaderProgram(
          {loaded_shaders_[vert_src_path], loaded_shaders_[frag_src_path]});
    } else {
      if (reload) {
        glDeleteProgram(loaded_programs_[shader_name]);
        loaded_programs_[shader_name] = LinkShaderProgram(
            {loaded_shaders_[vert_src_path], loaded_shaders_[frag_src_path]});
      }
    }
  }

  GLuint GetShader(const std::string& shader_name) const {
    if (loaded_programs_.find(shader_name) != loaded_programs_.end()) {
      return loaded_programs_.at(shader_name);
    }
    return 0;
  }

  std::map<std::string, GLuint> loaded_programs() const {
    return loaded_programs_;
  }

 private:
  std::string ReadFileToString(const char* file) {
    std::ifstream f(file);  // taking file as inputstream
    std::string str;
    if (f) {
      std::ostringstream ss;
      ss << f.rdbuf();  // reading data
      str = ss.str();
    }
    return str;
  }

  GLuint CompileShader(GLenum type, const char* vert_file) {
    std::string src = ReadFileToString(vert_file);
    GLuint shader = glCreateShader(type);
    const char* src_ptr = src.c_str();
    glShaderSource(shader, 1, &src_ptr, 0);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
      int log_len = 0;
      std::vector<char> log;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
      log.resize(log_len + 1, 0);
      glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
      printf("compile shader: %s error: %s\n", vert_file, log.data());
    }
    return shader;
  }

  GLuint LinkShaderProgram(std::vector<GLuint> shaders) {
    GLuint program = glCreateProgram();

    for (GLuint shader : shaders) {
      glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
      int log_len = 0;
      std::vector<char> log;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
      log.resize(log_len + 1, 0);
      glGetProgramInfoLog(program, log.size(), nullptr, log.data());
      printf("link program error: %s\n", log.data());
    }

    for (GLuint shader : shaders) {
      glDetachShader(program, shader);
    }
    return program;
  }

  std::map<std::string, GLuint> loaded_programs_;
  std::map<std::string, GLuint> loaded_shaders_;
};