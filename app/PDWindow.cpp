#include "PDWindow.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/stb_image.h"

namespace {

using SceneData = common::SceneData;
using ObjectData = common::ObjectData;
using milli = std::chrono::milliseconds;
namespace fs = std::experimental::filesystem;
using namespace nvgl;

std::set<std::string> extensions;
constexpr int kUniformBufferOffsetAlignment = 256;

int UniformBufferAlignedOffset(int size) {
  return (size + kUniformBufferOffsetAlignment - 1) /
         kUniformBufferOffsetAlignment * kUniformBufferOffsetAlignment;
}

// constexpr const char kMapDataFolder[] = "assets/dumped_map_data";
constexpr const char kMapDataFolder[] = "assets/dumped_map_data_compact";
constexpr const char kExtensionNVCommandList[] = "GL_NV_command_list";

class ProfileTimer {
 public:
  ProfileTimer(const std::string& entry_name) : entry_name_(entry_name) {
    start_ = std::chrono::high_resolution_clock::now();
  }
  ~ProfileTimer() {
    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << entry_name_ << " took "
              << std::chrono::duration_cast<milli>(finish - start_).count()
              << " milliseconds\n";
  }

 private:
  std::string entry_name_;
  std::chrono::time_point<std::chrono::system_clock> start_;
};

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

void LoadMapDataThreaded(const std::vector<fs::path>& files,
                         std::vector<std::unique_ptr<RenderObject>>& objects,
                         std::atomic_int& slot_idx) {
  while (true) {
    int current_slot_idx = slot_idx++;
    if (current_slot_idx >= files.size()) {
      break;
    }
    nlohmann::json json = LoadJsonFromFile(files[current_slot_idx]);
    if (json == nlohmann::json()) {
      printf("parsing json error: %s\n",
             files[current_slot_idx].string().c_str());
      continue;
    } else {
      // printf("parsing file: %s\n", directory_entry.path().string().c_str());
      auto object = CreateRenderObjectFromJson(json);
      if (object) {
        // object->Initialize();
        objects[current_slot_idx] = std::move(object);
      }
    }
  }
}

#define MULTI_THREAD
std::vector<std::unique_ptr<RenderObject>> LoadMapData(
    const std::string& map_directory) {
#ifdef MULTI_THREAD
  std::vector<fs::path> files;
  for (auto& directory_entry :
       fs::directory_iterator(fs::path(map_directory))) {
    files.push_back(directory_entry.path());
  }

  std::vector<std::unique_ptr<RenderObject>> objects(files.size());
  std::atomic_int slot_idx{0};
  const int kThreadCount = 15;
  std::vector<std::thread> threads;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.push_back(
        std::thread([&]() { LoadMapDataThreaded(files, objects, slot_idx); }));
  }
  LoadMapDataThreaded(files, objects, slot_idx);
  for (int i = 0; i < kThreadCount; ++i) {
    threads[i].join();
  }

  for (auto& object : objects) {
    object->Initialize();
  }
#else
  std::vector<std::unique_ptr<RenderObject>> objects;
  for (auto& directory_entry :
       fs::directory_iterator(fs::path(map_directory))) {
    nlohmann::json json = LoadJsonFromFile(directory_entry.path());
    if (json == nlohmann::json()) {
      printf("parsing json error: %s\n",
             directory_entry.path().string().c_str());
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
#endif
  return objects;
}

}  // namespace

PDWindow::~PDWindow() {}

PDWindow::PDWindow() : Window(u8"NVCommandListDemo") {}

void PDWindow::onInitialize() {
  Window::onInitialize();

  {
    ProfileTimer timer("LoadMapData");
    render_objects_ = LoadMapData(kMapDataFolder);
  }

  printf("total render object count:%d\n", render_objects_.size());

  ImGui::StyleColorsDark();
  GetGLExtension();

  int max_uniform_buffer_size = 0;
  glGetIntegerv(GL_UNIFORM_BUFFER_SIZE, &max_uniform_buffer_size);
  printf("max_uniform_buffer_size: %d\n", max_uniform_buffer_size);

  int uniform_buffer_offset_alignment = 0;
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
                &uniform_buffer_offset_alignment);
  printf("uniform_buffer_offset_alignment: %d\n",
         uniform_buffer_offset_alignment);

  command_list_supported_ = ExtensionSupport(kExtensionNVCommandList);

  // UBO
  glGenBuffers(1, &scene_ubo_);
  glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo_);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneData), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, scene_ubo_);

  glGenBuffers(1, &object_ubo_);
  glBindBuffer(GL_UNIFORM_BUFFER, object_ubo_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  // int data_stride = UniformBufferAlignedOffset(sizeof(ObjectData));
  // glBufferData(GL_UNIFORM_BUFFER, data_stride * render_objects_.size(),
  // nullptr,
  //              GL_DYNAMIC_DRAW);
  // glBindBuffer(GL_UNIFORM_BUFFER, 0);
  // glBindBufferBase(GL_UNIFORM_BUFFER, UBO_OBJECT, object_ubo_);

  program_manager_.m_filetype = nvh::ShaderFileManager::FILETYPE_GLSL;
  program_manager_.addDirectory("./assets/shaders/");
  program_manager_.addDirectory("./app/");

  program_manager_.registerInclude("common.h");

  ProgramID unlit_vertex_colored_id = program_manager_.createProgram(
      ProgramManager::Definition(GL_VERTEX_SHADER,
                                 "unlit_vertex_colored.vert.glsl"),
      ProgramManager::Definition(GL_FRAGMENT_SHADER,
                                 "unlit_vertex_colored.frag.glsl"));

  ProgramID unlit_colored_id = program_manager_.createProgram(
      ProgramManager::Definition(GL_VERTEX_SHADER,
                                 "unlit_colored_default.vert.glsl"),
      ProgramManager::Definition(GL_FRAGMENT_SHADER,
                                 "unlit_colored_default.frag.glsl"));

  ProgramID unlit_colored_uniform_id = program_manager_.createProgram(
      ProgramManager::Definition(GL_VERTEX_SHADER,
                                 "unlit_colored_uniform_buffer.vert.glsl"),
      ProgramManager::Definition(GL_FRAGMENT_SHADER,
                                 "unlit_colored_uniform_buffer.frag.glsl"));

  ProgramID simple_texture_object_id = program_manager_.createProgram(
      ProgramManager::Definition(GL_VERTEX_SHADER,
                                 "simple_textured_object.vert.glsl"),
      ProgramManager::Definition(GL_FRAGMENT_SHADER,
                                 "simple_textured_object.frag.glsl"));

  ProgramID simple_texture_object_uniform_id = program_manager_.createProgram(
      ProgramManager::Definition(
          GL_VERTEX_SHADER, "simple_textured_object_uniform_buffer.vert.glsl"),
      ProgramManager::Definition(
          GL_FRAGMENT_SHADER,
          "simple_textured_object_uniform_buffer.frag.glsl"));

  shader_manager_.RegisterShaderForName(
      "unlit_vertex_colored", program_manager_.get(unlit_vertex_colored_id));
  shader_manager_.RegisterShaderForName("unlit_colored",
                                        program_manager_.get(unlit_colored_id));
  shader_manager_.RegisterShaderForName(
      "unlit_colored_uniform", program_manager_.get(unlit_colored_uniform_id));
  shader_manager_.RegisterShaderForName(
      "simple_texture_colored", program_manager_.get(simple_texture_object_id));
  shader_manager_.RegisterShaderForName(
      "simple_texture_colored_uniform",
      program_manager_.get(simple_texture_object_uniform_id));

  glClearColor(0.1, 0.1, 0.1, 1);
  glClearDepth(1.0);
}

void PDWindow::onUpdate() {
  Window::onUpdate();
  gl_context_.glUseProgram(-1);
  gl_context_.glUseProgram(0);

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
  glm::mat4 projection = glm::perspective(
      glm::radians(60.0f), width / (float)height, 0.01f, 30000.0f);
  glm::mat4 view = camera_.view();

  scene_data_.VP = projection * view;

  glBindBuffer(GL_UNIFORM_BUFFER, scene_ubo_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneData), &scene_data_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  for (auto& k_v : shader_manager_.loaded_programs()) {
    gl_context_.glUseProgram(k_v.second);
    int VP_loc = glGetUniformLocation(k_v.second, "VP");
    glUniformMatrix4fv(VP_loc, 1, GL_FALSE, glm::value_ptr(scene_data_.VP));
    gl_context_.glUseProgram(0);
  }
}

void PDWindow::onRender() {
  ProfileTimer timer("OnRender");
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch (draw_method_) {
    case kBasic:
      DrawSceneBasic();
      break;
    case kBasicUniformBuffer:
      DrawSceneBasicUniformBuffer();
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
      "kBasicUniformBuffer",
      "Command List",
  };

  ImGui::Begin(u8"设置");
  int current_method = draw_method_;
  if (ImGui::Combo(u8"Draw Method", &current_method, combos, kMethodCount)) {
    draw_method_ = static_cast<DrawMethod>(current_method);
  }

  bool cache_state = gl_context_.cache_state();
  ImGui::Checkbox(u8"State Cache", &cache_state);
  gl_context_.set_cache_state(cache_state);

  ImGui::End();
}

void PDWindow::onResize(int w, int h) {
  Window::onResize(w, h);
  glViewport(0, 0, w, h);
}

void PDWindow::onEndFrame() { Window::onEndFrame(); }

void PDWindow::DrawSceneBasic() {
  // ProfileTimer timer("collect uniform data");
  auto pre_render_func =
      [&shader_manager_ = shader_manager_,
       &gl_context = gl_context_](RenderObject* render_object) -> bool {
    GLuint program = shader_manager_.GetShader(render_object->shader());
    gl_context.glUseProgram(program);

    int M_loc = glGetUniformLocation(program, "M");
    int color_loc = glGetUniformLocation(program, "color");
    int alpha_loc = glGetUniformLocation(program, "in_alpha");
    glUniformMatrix4fv(M_loc, 1, GL_FALSE,
                       glm::value_ptr(render_object->world()));

    auto line_object = dynamic_cast<LineObject*>(render_object);
    if (line_object) {
      glUniform4fv(color_loc, 1, glm::value_ptr(line_object->color()));
    }
    auto dashed_stripe_object =
        dynamic_cast<DashedStripeObject*>(render_object);
    if (dashed_stripe_object) {
      glUniform4fv(color_loc, 1, glm::value_ptr(dashed_stripe_object->color()));
    }
    auto simple_textured_object =
        dynamic_cast<SimpleTexturedObject*>(render_object);
    if (simple_textured_object) {
      glUniform1f(alpha_loc, simple_textured_object->alpha());
    }
    return true;
  };

  for (auto& object : render_objects_) {
    object->Render(shader_manager_, pre_render_func);
  }
}

void PDWindow::DrawSceneBasicUniformBuffer() {
  // Collect uniform data in buffer
  std::map<const void*, int> object_slot;
  std::vector<RenderObject*> real_render_objects;
  std::vector<ObjectData> object_datas;
  int data_stride = UniformBufferAlignedOffset(sizeof(ObjectData));
  {
    // ProfileTimer timer("collect uniform data");
    auto collect_data_pre_render_func =
        [&object_slot, &object_datas,
         &real_render_objects](RenderObject* render_object) -> bool {
      bool should_continue = true;
      ObjectData object_data;
      object_data.M = render_object->world();
      auto line_object = dynamic_cast<const LineObject*>(render_object);
      if (line_object) {
        object_data.color = line_object->color();
        should_continue = false;
      }
      auto dashed_stripe_object =
          dynamic_cast<const DashedStripeObject*>(render_object);
      if (dashed_stripe_object) {
        object_data.color = dashed_stripe_object->color();
        should_continue = false;
      }
      auto simple_textured_object =
          dynamic_cast<const SimpleTexturedObject*>(render_object);
      if (simple_textured_object) {
        object_data.color = glm::vec4(simple_textured_object->alpha());
        should_continue = false;
      }
      if (!should_continue) {
        object_datas.push_back(object_data);
        real_render_objects.push_back(render_object);
      }
      return should_continue;
    };

    for (auto& object : render_objects_) {
      object->Render(shader_manager_, collect_data_pre_render_func);
    }
  }

  {
    // ProfileTimer timer("upload uniform data");
    glBindBuffer(GL_UNIFORM_BUFFER, object_ubo_);
    if (object_ubo_size_ < object_datas.size() * data_stride) {
      glBufferData(GL_UNIFORM_BUFFER, object_datas.size() * data_stride, 0,
                   GL_DYNAMIC_DRAW);
      object_ubo_size_ = object_datas.size() * data_stride;
    }
    void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    for (int i = 0; i < object_datas.size(); ++i) {
      memcpy(ptr + data_stride * i, &object_datas[i], sizeof(ObjectData));
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  {
    // ProfileTimer timer("render data");

    for (int i = 0; i < real_render_objects.size(); ++i) {
      const RenderObject* object = real_render_objects[i];
      GLuint program = shader_manager_.GetShader(object->shader() + "_uniform");
      gl_context_.glUseProgram(program);
      glBindBufferRange(GL_UNIFORM_BUFFER, UBO_OBJECT, object_ubo_,
                        i * data_stride, sizeof(ObjectData));
      const_cast<RenderObject*>(object)->Render(shader_manager_);
    }
  }
}

void PDWindow::DrawSceneCommandList() {}

#undef min
#undef max
