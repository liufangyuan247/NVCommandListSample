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
int kMultiSampleCount = 8;

int UniformBufferAlignedOffset(int size) {
  return (size + kUniformBufferOffsetAlignment - 1) /
         kUniformBufferOffsetAlignment * kUniformBufferOffsetAlignment;
}

// constexpr const char kMapDataFolder[] = "assets/dumped_map_data";
constexpr const char kMapDataFolder[] = "assets/dumped_map_data_compact";
constexpr const char kExtensionNVCommandList[] = "GL_NV_command_list";
constexpr const char kExtensionARBBindlessTexture[] = "GL_ARB_bindless_texture";
constexpr const char kExtensionNVShaderBufferLoad[] = "GL_NV_shader_buffer_load";

const std::vector<std::string> kCommandListPrerequisiteExtensions = {
    kExtensionNVCommandList,
    kExtensionARBBindlessTexture,
    kExtensionNVShaderBufferLoad,
};

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

bool ExtensionsSupport(const std::vector<std::string>& extension_names) {
  for (const std::string& extension_name : extension_names) {
    if (!ExtensionSupport(extension_name)) {
      return false;
    }
  }
  return true;
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


std::vector<glm::vec3> points;
std::vector<float> times;
std::vector<glm::vec3> tangents;

float radius = 1000.0f;
int pointsCount = 200;
float cameraSpeed = 100.0f;

// 初始化Spline的控制点和对应的时间
void InitSpline(const float radius, const int pointsCount,
                const float cameraSpeed, std::vector<glm::vec3>& points,
                std::vector<float>& times, std::vector<glm::vec3>& tangents) {
  srand(1000);
  // 随机生成控制点
  for (int i = 0; i < pointsCount; i++) {
    float x = static_cast<float>(rand() % 200 - 100) / 100.0f;
    float y = static_cast<float>(rand() % 200 - 100) / 100.0f;
    float z = static_cast<float>(rand() % 200) / 200.0f;
    glm::vec3 point = glm::normalize(glm::vec3(x, y, z)) * radius;
    points.push_back(point);
  }

  // 计算切线向量
  for (int i = 0; i < points.size(); i++) {
    glm::vec3 tangent;
    if (i == 0) {
      tangent = glm::normalize(points[i + 1] - points[i]);
    } else if (i == points.size() - 1) {
      tangent = glm::normalize(points[i] - points[i - 1]);
    } else {
      tangent = glm::normalize((points[i + 1] - points[i - 1]) / 2.0f);
    }

    tangents.push_back(tangent);
  }

  // 计算每个控制点对应的时间值
  float time = 0.0;
  for (int i = 0; i < points.size() - 1; i++) {
    times.push_back(time);
    float distance = glm::length(points[i + 1] - points[i]);
    time += distance / cameraSpeed;
  }
}

// 根据给定的时间值，计算摄像机的位置和方向
void ComputeCameraPosition(const float time,
                           const std::vector<glm::vec3>& points,
                           const std::vector<float>& times,
                           const std::vector<glm::vec3>& tangents,
                           glm::vec3& position, glm::vec3& front) {
  // 保证时间值在合法范围内
  float T = glm::clamp(glm::mod(time, times.back()), 0.0f, times.back());

  // 找到time对应的区间
  int i = 0;
  while (T > times[i]) {
    i++;
  }

  // 根据控制点和切线向量计算插值系数
  float t = (T - times[i - 1]) / (times[i] - times[i - 1]);
  glm::vec3 p0 = points[i - 1];
  glm::vec3 p1 = points[i];
  glm::vec3 m0 = tangents[i - 1] * (times[i] - times[i - 1]);
  glm::vec3 m1 = tangents[i] * (times[i] - times[i - 1]);
  float t2 = t * t;
  float t3 = t2 * t;
  glm::vec3 A = 2.0f * p0 - 2.0f * p1 + m0 + m1;
  glm::vec3 B = -3.0f * p0 + 3.0f * p1 - 2.0f * m0 - m1;
  glm::vec3 C = m0;
  glm::vec3 D = p0;

  // 计算摄像机的位置和方向
  position = A * t3 + B * t2 + C * t + D;
  glm::vec3 dir = glm::mix(tangents[i - 1], tangents[i], t);
  dir.z = -glm::abs(dir.z);
  front = glm::normalize(dir);
}

}  // namespace

PDWindow::~PDWindow() {
  if (command_list_supported_) {
    FinalizeCommandListResouce();
  }
}

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

  command_list_supported_ = ExtensionsSupport(kCommandListPrerequisiteExtensions);

  if (command_list_supported_) {
    InitializeCommandListResouce();
  }

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

  InitSpline(radius, pointsCount, cameraSpeed, points, times, tangents);
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

  if (roaming_) {
    glm::vec3 pos;
    glm::vec3 dir;
    ComputeCameraPosition(Time::time(), points, times, tangents, pos, dir);
    float pitch = glm::degrees(glm::asin(dir.z));
    float yaw = glm::degrees(std::atan2(dir.x, dir.y));
    camera_.set_look_pitch_yaw({pitch, yaw});
    camera_.set_target(pos + dir * camera_.distance());
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
  if (command_list_supported_) {
    BindFallbackFramebuffer();
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  switch (draw_method_) {
    case kBasic:
      DrawSceneBasic();
      break;
    case kBasicUniformBuffer:
      DrawSceneBasicUniformBuffer();
      break;
    case kCommandToken:
      DrawSceneCommandToken();
      break;
    case kCommandList:
      DrawSceneCommandList();
      break;
  }
  if (command_list_supported_) {
    BlitFallbackFramebuffer();
  }
}

void PDWindow::onUIUpdate() {
  Window::onUIUpdate();
  ImGui::ShowDemoWindow();

  const char* combos[] = {
      "Normal",
      "kBasicUniformBuffer",
      "kCommandToken",
      "kCommandList",
  };

  ImGui::Begin(u8"设置");
  int current_method = draw_method_;
  if (ImGui::Combo(u8"Draw Method", &current_method, combos, kMethodCount)) {
    draw_method_ = static_cast<DrawMethod>(current_method);
  }

  bool cache_state = gl_context_.cache_state();
  ImGui::Checkbox(u8"State Cache", &cache_state);
  gl_context_.set_cache_state(cache_state);
  ImGui::Checkbox(u8"Romaing", &roaming_);

  ImGui::End();
}

void PDWindow::onResize(int w, int h) {
  Window::onResize(w, h);
  if (command_list_supported_) {
    ResizeCommandListRenderbuffers(w, h);
  }
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
  std::vector<RenderObject*> real_render_objects;
  std::vector<ObjectData> object_datas;
  int data_stride = UniformBufferAlignedOffset(sizeof(ObjectData));
  {
    // TODO: parallel collect data.
    // ProfileTimer timer("collect uniform data");
    auto collect_data_pre_render_func =
        [&object_datas,
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

void PDWindow::BindFallbackFramebuffer() {
  int real_sample_count = 0;
  glGetIntegerv(GL_SAMPLES, &real_sample_count);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (int*)&command_list_data_.original_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, command_list_data_.fallback_framebuffer);
  if (real_sample_count != kMultiSampleCount) {
    kMultiSampleCount = real_sample_count;
    ResizeCommandListRenderbuffers(width, height);
  }
}

void PDWindow::BlitFallbackFramebuffer() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, command_list_data_.fallback_framebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, command_list_data_.original_framebuffer);
  glBlitFramebuffer(0 ,0 ,width, height, 0 ,0 ,width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void PDWindow::DrawSceneCommandToken() {
  if (!command_list_supported_) {
    return;
  }

  // Record draw commands
  std::vector<ObjectData> object_datas;
  std::vector<GLuint> states;
  std::vector<GLuint> fbos;
  std::string token_buffer;
  NVTokenSequence& token_sequence = command_list_data_.token_sequence;
  
  std::vector<RenderObject*> real_render_objects;

  {
    // TODO: parallel collect data.
    // ProfileTimer timer("collect uniform data");
    auto collect_data_pre_render_func =
        [&object_datas,
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

  int data_stride = UniformBufferAlignedOffset(sizeof(ObjectData));
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

  // Play draw commands

  // Delete all states
  glDeleteStatesNV(states.size(), states.data());

}

void PDWindow::DrawSceneCommandList() {
  if (!command_list_supported_) {
    return;
  }
}

void PDWindow::ResizeCommandListRenderbuffers(int w, int h) {
  if (command_list_data_.color_texture) {
    glDeleteTextures(1, &command_list_data_.color_texture);
    glDeleteTextures(1, &command_list_data_.depth_stencil_texture);

    glMakeTextureHandleNonResidentARB(command_list_data_.color_texture_handle);
    glMakeTextureHandleNonResidentARB(
        command_list_data_.depth_stencil_texture_handle);
  }
  glGenTextures(1, &command_list_data_.color_texture);
  glGenTextures(1, &command_list_data_.depth_stencil_texture);

  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, command_list_data_.color_texture);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, kMultiSampleCount,
                          GL_RGBA8, w, h, GL_TRUE);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE,
                command_list_data_.depth_stencil_texture);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, kMultiSampleCount,
                          GL_DEPTH24_STENCIL8, w, h, GL_TRUE);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, command_list_data_.fallback_framebuffer);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                       command_list_data_.color_texture, 0);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                       command_list_data_.depth_stencil_texture, 0);
  GLenum draw_buffer{GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, &draw_buffer);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("command list framebuffer incomplete!!\n");
  } else {
    printf("command list framebuffer complete!!\n");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  command_list_data_.color_texture_handle =
      glGetTextureHandleARB(command_list_data_.color_texture);
  command_list_data_.depth_stencil_texture_handle =
      glGetTextureHandleARB(command_list_data_.depth_stencil_texture);
  printf("color_texture_handle:%x  depth_stencil_texture_handle:%x\n", command_list_data_.color_texture_handle,
  command_list_data_.depth_stencil_texture_handle);
  glMakeTextureHandleResidentARB(command_list_data_.color_texture_handle);
  glMakeTextureHandleResidentARB(
      command_list_data_.depth_stencil_texture_handle);
}

void PDWindow::InitializeCommandListResouce() {
  glGenFramebuffers(1, &command_list_data_.fallback_framebuffer);

  ResizeCommandListRenderbuffers(width, height);
}

void PDWindow::FinalizeCommandListResouce() {
  glDeleteFramebuffers(1, &command_list_data_.fallback_framebuffer);
  glDeleteTextures(1, &command_list_data_.color_texture);
  glDeleteTextures(1, &command_list_data_.depth_stencil_texture);

  glMakeTextureHandleNonResidentARB(command_list_data_.color_texture_handle);
  glMakeTextureHandleNonResidentARB(
      command_list_data_.depth_stencil_texture_handle);
}

#undef min
#undef max
