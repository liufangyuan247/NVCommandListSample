#include "PDWindow.h"

#include <cstdio>
#include <experimental/filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/stb_image.h"

namespace {

namespace fs = std::experimental::filesystem;
std::set<std::string> extensions;

constexpr const char kMapDataFolder[] = "assets/dumped_map_data";
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

nlohmann::json LoadJsonFromFile(const fs::path& path) {
  nlohmann::json json;
  std::ifstream ifs(path.string());
  if (ifs) {
    ifs >> json;
  } else {
    printf("open file error: %s\n", path.string().c_str());
  }
  return json;
}

std::vector<std::unique_ptr<RenderObject>> LoadMapData(
    const std::string& map_directory) {
  std::vector<std::unique_ptr<RenderObject>> objects;
  for (auto& directory_entry :
       fs::directory_iterator(fs::path(map_directory))) {
    nlohmann::json json = LoadJsonFromFile(directory_entry.path());
    if (json == nlohmann::json()) {
      printf("parsing json error: %s\n", directory_entry.path().string().c_str());
      continue;
    } else {
      // printf("parsing file: %s\n", directory_entry.path().string().c_str());
      auto object = CreateRenderObjectFromJson(json);
      if (object) {
        object->Initialize();
        objects.push_back(std::move(object));
      }
    }
  }
  return objects;
}

}  // namespace

PDWindow::~PDWindow() {}

PDWindow::PDWindow() : Window(u8"NVCommandListDemo") {}

void PDWindow::onInitialize() {
  Window::onInitialize();

  render_objects_ = LoadMapData(kMapDataFolder);
  printf("total render object count:%d\n", render_objects_.size());

  ImGui::StyleColorsDark();
  GetGLExtension();

  int max_uniform_buffer_size = 0;
  glGetIntegerv(GL_UNIFORM_BUFFER_SIZE, &max_uniform_buffer_size);
  printf("max_uniform_buffer_size: %d\n", max_uniform_buffer_size);

  int uniform_buffer_offset_alignment = 0;
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_offset_alignment);
  printf("uniform_buffer_offset_alignment: %d\n", uniform_buffer_offset_alignment);

  command_list_supported_ = ExtensionSupport(kExtensionNVCommandList);

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

  shader_manager_.LoadShaderForName(
      "unlit_vertex_colored", "assets/shaders/unlit_vertex_colored.vert.glsl",
      "assets/shaders/unlit_vertex_colored.frag.glsl");
  shader_manager_.LoadShaderForName(
      "unlit_colored", "assets/shaders/unlit_colored_default.vert.glsl",
      "assets/shaders/unlit_colored_default.frag.glsl");
  shader_manager_.LoadShaderForName("unlit_vertex_colored_command_list",
                                    "assets/shaders/colored_cl.vert.glsl",
                                    "assets/shaders/colored_cl.frag.glsl");
  shader_manager_.LoadShaderForName(
      "simple_texture_object", "assets/shaders/simple_textured_object.vert.glsl",
      "assets/shaders/simple_textured_object.frag.glsl");

  glClearColor(0.1, 0.1, 0.1, 1);
  glClearDepth(1.0);
}

void PDWindow::onUpdate() {
  Window::onUpdate();

  // Process camera update
  {
    glm::vec3 forward = camera_.forward();
    glm::vec3 right = camera_.right();

    float speed = input.Shift() ? 100.0f : 1.0f;
    float dis = speed * Time::deltaTime();

    glm::vec3 target = camera_.target();
    if (input.getKey(GLFW_KEY_A)) {
      target -= right * dis;
    }
    if (input.getKey(GLFW_KEY_D)) {
      target += right * dis;
    }
    if (input.getKey(GLFW_KEY_W)) {
      target += forward * dis;
    }
    if (input.getKey(GLFW_KEY_S)) {
      target -= forward * dis;
    }
    camera_.set_target(target);

    if (input.getButton(GLFW_MOUSE_BUTTON_LEFT) && !input.blockedByUI()) {
      const float sensitivity = 0.1f;
      glm::vec2 picth_yaw = camera_.look_pitch_yaw();
      picth_yaw += glm::vec2{-input.deltaY(), input.deltaX()} * sensitivity;
      picth_yaw.x = glm::clamp(picth_yaw.x, -90.0f, 90.0f);
      camera_.set_look_pitch_yaw(picth_yaw);
    }
  }

  // Compute VP matrix
  glm::mat4 projection = glm::perspective(glm::radians(60.0f), width / (float) height, 0.01f, 5000.0f);
  glm::mat4 view = camera_.view();

  scene_data_.VP = projection * view;

  glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &scene_data_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  for (auto& k_v : shader_manager_.loaded_programs()) {
    glUseProgram(k_v.second);
    int VP_loc = glGetUniformLocation(k_v.second, "VP");
    glUniformMatrix4fv(VP_loc, 1, GL_FALSE, glm::value_ptr(scene_data_.VP));
    glUseProgram(0);
  }
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
  ImGui::ShowDemoWindow();

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
  //object_data_.M = glm::rotate(glm::mat4(1.0), Time::time(), glm::vec3(0, 1, 0));
  object_data_.M = glm::mat4(1.0);

  glBindBuffer(GL_UNIFORM_BUFFER, object_ubo_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ObjectData), &object_data_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  for (auto& object : render_objects_) {
    object->Render(shader_manager_);
  }
}

void PDWindow::DrawSceneCommandList() {

}

#undef min
#undef max
