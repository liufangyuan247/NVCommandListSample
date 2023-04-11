#include "PDWindow.h"

#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/stb_image.h"

namespace {

std::set<std::string> extensions;

constexpr const char kExtensionNVCommandList[] = "GL_NV_command_list";

void GetGLExtension() {
  int extension_num = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &extension_num);
  for (int i = 0; i < extension_num; ++i) {
    std::string extension =
        reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
    extensions.insert(extension);
    // printf("%s\n", extension.c_str());
  }
}

bool ExtensionSupport(const std::string& extension_name) {
  return extensions.find(extension_name) != extensions.end();
}

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

GLuint LoadShaderProgram(const char* vert_file, const char* frag_file) {
  std::string vert_src = ReadFileToString(vert_file);
  std::string frag_src = ReadFileToString(frag_file);
  
  GLuint vert_shader = CompileShader(GL_VERTEX_SHADER, vert_file);
  GLuint frag_shader = CompileShader(GL_FRAGMENT_SHADER, frag_file);
  
  GLuint program = glCreateProgram();
  glAttachShader(program, vert_shader);
  glAttachShader(program, frag_shader);

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

  glDetachShader(program, vert_shader);
  glDetachShader(program, frag_shader);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  return program;
}

}  // namespace

PDWindow::~PDWindow() {}

PDWindow::PDWindow() : Window(u8"NVCommandListDemo") {}

void PDWindow::onInitialize() {
  Window::onInitialize();
  ImGui::StyleColorsDark();
  GetGLExtension();

  command_list_supported_ = ExtensionSupport(kExtensionNVCommandList);

  // Initialize mesh data
  struct VertexAttribs {
    glm::vec3 pos;
    unsigned char color[4];
  };

  VertexAttribs mesh[] = {
    {{0, 1, 0}, {255, 0, 0, 255} },
    {{-1, -1, 0}, {0, 255, 0, 255} },
    {{1, -1, 0}, {0, 0, 255, 255} },    
  };

  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(mesh), &mesh, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttribs), 0);
  glVertexAttribDivisor(0,0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexAttribs), (void*)(sizeof(glm::vec3)));

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // UBO
  glGenBuffers(1, &scene_ubo_);
  glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo_);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneData), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene_ubo_);

  glGenBuffers(1, &object_ubo_);
  glBindBuffer(GL_UNIFORM_BUFFER, object_ubo_);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(ObjectData), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, object_ubo_);

  // Load shaders
  normal_shader_ = LoadShaderProgram("assets/shaders/colored.vert.glsl", "assets/shaders/colored.frag.glsl");
  command_list_shader_ = LoadShaderProgram("assets/shaders/colored_cl.vert.glsl", "assets/shaders/colored_cl.frag.glsl");

  shader_name_["unlit_color"] = normal_shader_;

  glClearColor(0.5, 0.5, 0.5, 1);
  glClearDepth(1.0);
}

void PDWindow::onUpdate() {
  Window::onUpdate();

  // Compute VP matrix
  glm::mat4 projection = glm::perspective(glm::radians(60.0f), width / (float) height, 0.1f, 5.0f);
  glm::mat4 view = glm::lookAt(glm::vec3(0, 0, -2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  scene_data_.VP = projection * view;

  glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &scene_data_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void PDWindow::onRender() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch (draw_method_) {
    case kBasic:
      DrawSceneBasic();
      break;
    case kCommandList:
      DrawSceneCommandList();
      break;
  }
}

void PDWindow::onUIUpdate() {
  Window::onUIUpdate();
  const char* combos[] = {
    "Normal",
    "Command List",
  };

  ImGui::Begin(u8"设置");
  int current_method = draw_method_;
  if (ImGui::Combo(u8"Draw Method", &current_method, combos, 2)) {
    draw_method_ = static_cast<DrawMethod>(current_method);
  }

  ImGui::End();
}

void PDWindow::onResize(int w, int h) {
  Window::onResize(w, h);
  glViewport(0, 0, w, h);
}

void PDWindow::onEndFrame() { Window::onEndFrame(); }

void PDWindow::DrawSceneBasic() {
  object_data_.M = glm::rotate(glm::mat4(1.0), Time::time(), glm::vec3(0, 1, 0));

  glBindBuffer(GL_UNIFORM_BUFFER, object_ubo_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ObjectData), &object_data_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glUseProgram(normal_shader_);
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
  glUseProgram(0);
}

void PDWindow::DrawSceneCommandList() {

}

#undef min
#undef max
