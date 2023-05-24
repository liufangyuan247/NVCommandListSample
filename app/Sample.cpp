#include "Sample.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "app/extension_command_list.h"
#include "core/stb_image.h"

namespace {

using SceneData = common::SceneData;
using ObjectData = common::ObjectData;
using MaterialData = common::MaterialData;
using us = std::chrono::microseconds;
namespace fs = std::experimental::filesystem;
using namespace nvgl;

class ProfileTimerGroup {
 public:
  ProfileTimerGroup(const std::string& entry_name) : entry_name_(entry_name) {}
  ~ProfileTimerGroup() {
    printf("%s took %.2f ms\n", entry_name_.c_str(),
           std::chrono::duration_cast<us>(elapsed_time_).count() * 0.001f);
  }

  void AddTime(
      const std::chrono::high_resolution_clock::duration& elapsed_time) {
    elapsed_time_ += elapsed_time;
  }

 private:
  std::string entry_name_;
  std::chrono::high_resolution_clock::duration elapsed_time_{0};
  std::chrono::time_point<std::chrono::system_clock> start_;
};

class ProfileTimer {
 public:
  ProfileTimer(const std::string& entry_name,
               ProfileTimerGroup* group = nullptr)
      : entry_name_(entry_name), group_(group) {
    start_ = std::chrono::high_resolution_clock::now();
  }
  ~ProfileTimer() {
    auto finish = std::chrono::high_resolution_clock::now();
    if (!group_) {
      printf("%s took %.2f ms\n", entry_name_.c_str(),
             std::chrono::duration_cast<us>(finish - start_).count() * 0.001f);
    } else {
      group_->AddTime(finish - start_);
    }
  }

 private:
  std::string entry_name_;
  std::chrono::time_point<std::chrono::system_clock> start_;
  ProfileTimerGroup* group_ = nullptr;
};

std::set<std::string> extensions;
constexpr int kUniformBufferOffsetAlignment = 256;
int kMultiSampleCount = 8;

int UniformBufferAlignedOffset(int size) {
  return (size + kUniformBufferOffsetAlignment - 1) /
         kUniformBufferOffsetAlignment * kUniformBufferOffsetAlignment;
}

template <typename Command>
void PushCommandToBuffer(const Command& command, std::string* buffer) {
  buffer->insert(buffer->end(), (const char*)(&command),
                 (const char*)(&command) + sizeof(Command));
}

// constexpr const char kMapDataFolder[] = "assets/dumped_map_data";
constexpr const char kMapDataFolder[] = "assets/dumped_map_data_compact";
constexpr const char kExtensionNVCommandList[] = "GL_NV_command_list";
constexpr const char kExtensionARBBindlessTexture[] = "GL_ARB_bindless_texture";
constexpr const char kExtensionNVShaderBufferLoad[] =
    "GL_NV_shader_buffer_load";

const std::vector<std::string> kCommandListPrerequisiteExtensions = {
    kExtensionNVCommandList,
    kExtensionARBBindlessTexture,
    kExtensionNVShaderBufferLoad,
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

GLenum GetBaseDrawMode(GLenum draw_mode) {
  switch (draw_mode) {
    case GL_POINTS:
      return GL_POINTS;
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
      return GL_LINES;
    case GL_TRIANGLES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
      return GL_TRIANGLES;
  }
  return draw_mode;
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
                         std::atomic_int& read_slot_idx,
                         std::atomic_int& write_slot_idx) {
  while (true) {
    int current_slot_idx = read_slot_idx++;
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
        int write_idx = write_slot_idx++;
        objects[write_idx] = std::move(object);
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
  // files.resize(100);

  std::vector<std::unique_ptr<RenderObject>> objects(files.size());
  std::atomic_int read_slot_idx{0};
  std::atomic_int write_slot_idx{0};
  const int kThreadCount = 15;
  std::vector<std::thread> threads;

  for (int i = 0; i < kThreadCount; ++i) {
    threads.push_back(
        std::thread([&]() { LoadMapDataThreaded(files, objects, read_slot_idx, write_slot_idx); }));
  }
  LoadMapDataThreaded(files, objects, read_slot_idx, write_slot_idx);
  for (int i = 0; i < kThreadCount; ++i) {
    threads[i].join();
  }

  objects.resize(write_slot_idx);

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

// 初始化Spline的控制点和对应的时间
void InitSpline(const float radius, const int pointsCount,
                const float camera_speed_, std::vector<glm::vec3>& points,
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
    time += distance / camera_speed_;
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

void PrintOpenGLCapablities() {
  GLint uboSize = 0;
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uboSize);
  printf("max uniform block size: %d\n", uboSize);

  int uniform_buffer_offset_alignment = 0;
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
                &uniform_buffer_offset_alignment);
  printf("uniform_buffer_offset_alignment: %d\n",
         uniform_buffer_offset_alignment);
}

GLuint LoadTexture(const char* path) {
  GLuint texture = 0;
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* pixels = stbi_load(path, &width, &height, &channels, 4);
  if (pixels) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);
  } else {
    printf("failed to load texture: %s\n", path);
  }
  return texture;
}

}  // namespace

CommandListSample::~CommandListSample() {
  if (command_list_supported_) {
    FinalizeCommandListResouce();
  }
}

CommandListSample::CommandListSample() : Window(u8"NVCommandListSample") {}

void CommandListSample::onInitialize() {
  Window::onInitialize();

  {
    ProfileTimer timer("LoadMapData");
    render_objects_ = LoadMapData(kMapDataFolder);
  }

  printf("total render object count:%d\n", render_objects_.size());

  ImGui::StyleColorsDark();
  GetGLExtension();

  PrintOpenGLCapablities();

  command_list_supported_ =
      ExtensionsSupport(kCommandListPrerequisiteExtensions);

  if (command_list_supported_) {
    InitializeCommandListResouce();
  }

  texture_[0] = LoadTexture("assets/textures/uvtest.jpg");
  texture_[1] = LoadTexture("assets/textures/uvtest.png");

  if (command_list_supported_) {
    for (int i = 0; i < 2; ++i) {
      texture_address_[i] = glGetTextureHandleARB(texture_[i]);
      material_data_[i].texture = texture_address_[i];
      glMakeTextureHandleResidentNV(texture_address_[i]);
    }
  }

  // UBO
  glCreateBuffers(1, &scene_ubo_);
  glNamedBufferData(scene_ubo_, sizeof(SceneData), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, scene_ubo_);

  if (command_list_supported_) {
    glGetNamedBufferParameterui64vNV(scene_ubo_, GL_BUFFER_GPU_ADDRESS_NV,
                                     &scene_ubo_address_);
    glMakeNamedBufferResidentNV(scene_ubo_, GL_READ_ONLY);
  }

  glCreateBuffers(1, &material_ubo_);
  glNamedBufferData(material_ubo_,
                    UniformBufferAlignedOffset(sizeof(MaterialData)) * 2,
                    nullptr, GL_STATIC_DRAW);
  glNamedBufferSubData(material_ubo_, 0, sizeof(MaterialData),
                       &material_data_[0]);
  glNamedBufferSubData(material_ubo_,
                       UniformBufferAlignedOffset(sizeof(MaterialData)),
                       sizeof(MaterialData), &material_data_[1]);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATERIAL, material_ubo_);

  if (command_list_supported_) {
    glGetNamedBufferParameterui64vNV(material_ubo_, GL_BUFFER_GPU_ADDRESS_NV,
                                     &material_ubo_address_);
    glMakeNamedBufferResidentNV(material_ubo_, GL_READ_ONLY);
  }

  glCreateBuffers(1, &object_ubo_);

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
      "simple_textured_object", program_manager_.get(simple_texture_object_id));
  shader_manager_.RegisterShaderForName(
      "simple_textured_object_uniform",
      program_manager_.get(simple_texture_object_uniform_id));

  glClearColor(0.1, 0.1, 0.1, 1);
  glClearDepth(1.0);

  InitSpline(radius, pointsCount, camera_speed_, points, times, tangents);
}

void CommandListSample::onUpdate() {
  Window::onUpdate();
  gl_context_.glUseProgram(0);

  // Process camera update
  {
    glm::vec3 forward = camera_.forward();
    glm::vec3 right = camera_.right();

    float speed = input.Shift() ? camera_speed_ * 100.0f : camera_speed_;
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

  glNamedBufferSubData(scene_ubo_, 0, sizeof(SceneData), &scene_data_);

  for (auto& k_v : shader_manager_.loaded_programs()) {
    gl_context_.glUseProgram(k_v.second);
    int VP_loc = glGetUniformLocation(k_v.second, "VP");
    if (VP_loc != -1) {
      glUniformMatrix4fv(VP_loc, 1, GL_FALSE, glm::value_ptr(scene_data_.VP));
    }
    gl_context_.glUseProgram(0);
  }
}

void CommandListSample::onRender() {
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

void CommandListSample::onUIUpdate() {
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

  ImGui::DragFloat(u8"camera speed", &camera_speed_, 1.0);

  bool cache_state = gl_context_.cache_state();
  ImGui::Checkbox(u8"State Cache", &cache_state);
  gl_context_.set_cache_state(cache_state);
  ImGui::Checkbox(u8"Romaing", &roaming_);

  ImGui::Checkbox(u8"Selective Draw", &selective_draw_);
  ImGui::DragInt(u8"Selective Draw Start", &selective_draw_start_, 1, 0,
                 command_list_data_.token_sequence.offsets.size());
  ImGui::DragInt(u8"Selective Draw Count", &selective_draw_count_, 1, 0,
                 command_list_data_.token_sequence.offsets.size());

  ImGui::Text("total uniform buffer size: %fMB",
              object_ubo_size_ / 1024.0f / 1024.0f);
  ImGui::Text("total states: %d", state_caches_.size());
  ImGui::Text("total token sequence count: %d",
              command_list_data_.token_sequence.offsets.size());
  ImGui::Text(
      "total command token buffer size: %fMB",
      command_list_data_.command_stream_buffer_size / 1024.0f / 1024.0f);
  ImGui::Text("total road graph element count: %d", render_objects_.size());

  ImGui::End();
}

void CommandListSample::onResize(int w, int h) {
  Window::onResize(w, h);
  if (command_list_supported_) {
    ResizeCommandListRenderbuffers(w, h);
  }
  glViewport(0, 0, w, h);
}

void CommandListSample::onEndFrame() { Window::onEndFrame(); }

void CommandListSample::DrawSceneBasic() {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_[0]);

  // ProfileTimer timer("collect uniform data");
  auto pre_render_func =
      [&shader_manager_ = shader_manager_,
       &gl_context = gl_context_](RenderObject* render_object) -> bool {
    GLuint program = shader_manager_.GetShader(render_object->shader());
    gl_context.glUseProgram(program);

    int M_loc = program ? glGetUniformLocation(program, "M") : -1;
    int color_loc = program ? glGetUniformLocation(program, "color") : -1;
    int alpha_loc = program ? glGetUniformLocation(program, "in_alpha") : -1;
    if (M_loc != -1) {
      glUniformMatrix4fv(M_loc, 1, GL_FALSE,
                         glm::value_ptr(render_object->world()));
    }
    int tex0_loc = program ? glGetUniformLocation(program, "tex0") : -1;
    if (M_loc != -1) {
      glUniform1i(tex0_loc, 0);
    }
    auto line_object = dynamic_cast<LineObject*>(render_object);
    if (line_object && color_loc != -1) {
      glUniform4fv(color_loc, 1, glm::value_ptr(line_object->color()));
    }
    auto dashed_stripe_object =
        dynamic_cast<DashedStripeObject*>(render_object);
    if (dashed_stripe_object && color_loc != -1) {
      glUniform4fv(color_loc, 1, glm::value_ptr(dashed_stripe_object->color()));
    }
    auto simple_textured_object =
        dynamic_cast<SimpleTexturedObject*>(render_object);
    if (simple_textured_object && alpha_loc != -1) {
      glUniform1f(alpha_loc, simple_textured_object->alpha());
    }
    return true;
  };

  for (auto& object : render_objects_) {
    object->Render(shader_manager_, pre_render_func);
  }
}

void CommandListSample::DrawSceneBasicUniformBuffer() {
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
    if (object_ubo_size_ < object_datas.size() * data_stride) {
      glNamedBufferData(object_ubo_, object_datas.size() * data_stride, 0,
                        GL_DYNAMIC_DRAW);
      object_ubo_size_ = object_datas.size() * data_stride;
      object_ubo_address_ = 0;
    }
    unsigned char* ptr =
        (unsigned char*)glMapNamedBuffer(object_ubo_, GL_WRITE_ONLY);
    for (int i = 0; i < object_datas.size(); ++i) {
      memcpy(ptr + data_stride * i, &object_datas[i], sizeof(ObjectData));
    }
    glUnmapNamedBuffer(object_ubo_);
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

void CommandListSample::BindFallbackFramebuffer() {
  int real_sample_count = 0;
  glGetIntegerv(GL_SAMPLES, &real_sample_count);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                (int*)&command_list_data_.original_framebuffer);
  if (real_sample_count != kMultiSampleCount) {
    kMultiSampleCount = real_sample_count;
    ResizeCommandListRenderbuffers(width, height);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, command_list_data_.fallback_framebuffer);
}

void CommandListSample::BlitFallbackFramebuffer() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER,
                    command_list_data_.fallback_framebuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                    command_list_data_.original_framebuffer);
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void CommandListSample::CollectRenderObjectData(
    std::vector<ObjectData>& object_datas,
    std::vector<RenderObject*>& real_render_objects,
    std::vector<CapturedStateCache>& render_object_states) {
  // TODO: parallel collect data.
  ProfileTimer timer("  collect render datas");
  auto collect_data_pre_render_func = [&object_datas, &real_render_objects,
                                       &render_object_states,
                                       &shader_manager_ = shader_manager_](
                                          RenderObject* render_object) -> bool {
    bool should_continue = true;
    ObjectData object_data;
    CapturedStateCache render_state;
    render_state.program =
        shader_manager_.GetShader(render_object->shader() + "_uniform");
    render_state.vertex_attrib_mask =
        render_object->mesh_renderer().vertex_attrib_mask();
    render_state.base_draw_mode =
        GetBaseDrawMode(render_object->mesh_renderer().mesh().draw_mode());

    object_data.M = render_object->world();
    auto line_object = dynamic_cast<const LineObject*>(render_object);
    if (line_object) {
      object_data.color = line_object->color();
      should_continue = false;
      render_state.enable_line_stipple =
          (uint8_t)line_object->line_style().line_stipple;
      render_state.stipple_factor = line_object->line_style().line_stipple_factor;
      render_state.stipple_pattern = line_object->line_style().line_stipple_pattern;
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
      render_object_states.push_back(render_state);
    }
    return should_continue;
  };

  for (auto& object : render_objects_) {
    object->Render(shader_manager_, collect_data_pre_render_func);
  }
}

void CommandListSample::CompileDrawCommandList() {
  if (command_list_data_.draw_commands_compiled) {
    return;
  }

  // Record draw commands
  std::string& token_buffer = command_list_data_.command_stream_buffer_cpu_;
  token_buffer.clear();
  NVTokenSequence& token_sequence = command_list_data_.token_sequence;

  std::vector<ObjectData> object_datas;
  std::vector<RenderObject*> real_render_objects;
  std::vector<CapturedStateCache> render_object_states;
  CollectRenderObjectData(object_datas, real_render_objects,
                          render_object_states);

  ProfileTimer timer("  record render commands");

  token_sequence.offsets.resize(object_datas.size());
  token_sequence.sizes.resize(object_datas.size());
  token_sequence.states.resize(object_datas.size());
  token_sequence.fbos.resize(object_datas.size(),
                             command_list_data_.fallback_framebuffer);

  GLuint texture_shader = shader_manager_.GetShader("simple_textured_object_uniform");

  // Setup token buffer
  int data_stride = UniformBufferAlignedOffset(sizeof(ObjectData));
  {
    if (object_ubo_size_ < object_datas.size() * data_stride) {
      object_ubo_size_ = object_datas.size() * data_stride;
      glNamedBufferData(object_ubo_, object_ubo_size_, 0, GL_DYNAMIC_DRAW);
      object_ubo_address_ = 0;
    }
    if (!object_ubo_address_) {
      glGetNamedBufferParameterui64vNV(object_ubo_, GL_BUFFER_GPU_ADDRESS_NV,
                                       &object_ubo_address_);
      glMakeNamedBufferResidentNV(object_ubo_, GL_READ_ONLY);
    }

    // FIXME? State capture procedure will interfere with the object_ubo_
    // mapping
    unsigned char* ptr =
        (unsigned char*)glMapNamedBuffer(object_ubo_, GL_WRITE_ONLY);
    for (int i = 0; i < object_datas.size(); ++i) {
      memcpy(ptr + data_stride * i, &object_datas[i], sizeof(ObjectData));
    }
    glUnmapNamedBuffer(object_ubo_);

    // Build token buffer
    for (int i = 0; i < object_datas.size(); ++i) {
      token_sequence.states[i] = CaptureState(render_object_states[i]);

      const RenderObject* object = real_render_objects[i];
      auto line_object = dynamic_cast<const LineObject*>(object);

      // record draw command
      GLintptr offset = token_buffer.size();

      // Set up uniform binding info
      {
        uint header = glGetCommandHeaderNV(GL_UNIFORM_ADDRESS_COMMAND_NV,
                                           sizeof(UniformAddressCommandNV));
        uint64_t uniform_buffer_address = object_ubo_address_ + i * data_stride;
        PushCommandToBuffer(
            UniformAddressCommandNV{header, UBO_OBJECT,
                                    glGetStageIndexNV(GL_VERTEX_SHADER),
                                    uniform_buffer_address},
            &token_buffer);
        PushCommandToBuffer(
            UniformAddressCommandNV{header, UBO_OBJECT,
                                    glGetStageIndexNV(GL_FRAGMENT_SHADER),
                                    uniform_buffer_address},
            &token_buffer);
        PushCommandToBuffer(
            UniformAddressCommandNV{header, UBO_SCENE,
                                    glGetStageIndexNV(GL_VERTEX_SHADER),
                                    scene_ubo_address_},
            &token_buffer);
        PushCommandToBuffer(
            UniformAddressCommandNV{header, UBO_SCENE,
                                    glGetStageIndexNV(GL_FRAGMENT_SHADER),
                                    scene_ubo_address_},
            &token_buffer);

        if (render_object_states[i].program == texture_shader) {
          static int flip = 0;
          PushCommandToBuffer(
              UniformAddressCommandNV{
                  header, UBO_MATERIAL, glGetStageIndexNV(GL_VERTEX_SHADER),
                  material_ubo_address_ +
                      flip * UniformBufferAlignedOffset(sizeof(MaterialData))},
              &token_buffer);
          PushCommandToBuffer(
              UniformAddressCommandNV{
                  header, UBO_MATERIAL, glGetStageIndexNV(GL_FRAGMENT_SHADER),
                  material_ubo_address_ +
                      flip * UniformBufferAlignedOffset(sizeof(MaterialData))},
              &token_buffer);
          flip = 1 - flip;          
        }
      }

      // Set up vertex attrib binding info
      {
        uint header = glGetCommandHeaderNV(GL_ATTRIBUTE_ADDRESS_COMMAND_NV,
                                           sizeof(AttributeAddressCommandNV));
        PushCommandToBuffer(
            AttributeAddressCommandNV{header, 0,
                                      object->mesh_renderer().vbo_address()},
            &token_buffer);
      }
      // Set up index binding info
      if (object->mesh_renderer().mesh().indexed_draw()) {
        uint header = glGetCommandHeaderNV(GL_ELEMENT_ADDRESS_COMMAND_NV,
                                           sizeof(ElementAddressCommandNV));
        PushCommandToBuffer(
            ElementAddressCommandNV{header,
                                    object->mesh_renderer().ibo_address(),
                                    sizeof(Mesh::IndexType)},
            &token_buffer);
      }

      // Set up aux info
      if (line_object) {
        uint header = glGetCommandHeaderNV(GL_LINE_WIDTH_COMMAND_NV,
                                           sizeof(LineWidthCommandNV));
        float line_width = glm::clamp(line_object->line_style().line_width, 0.5f, 10.0f);
        PushCommandToBuffer(
            LineWidthCommandNV{header, line_width},
            &token_buffer);
      }

      // Set up draw command
      if (object->mesh_renderer().mesh().indexed_draw()) {
        uint header =
            glGetCommandHeaderNV(GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV,
                                 sizeof(DrawElementsInstancedCommandNV));
        PushCommandToBuffer(
            DrawElementsInstancedCommandNV{
                header, object->mesh_renderer().mesh().draw_mode(),
                (unsigned int)object->mesh_renderer().mesh().indices().size(),
                1, 0, 0, 0},
            &token_buffer);
      } else {
        uint header =
            glGetCommandHeaderNV(GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV,
                                 sizeof(DrawArraysInstancedCommandNV));
        PushCommandToBuffer(
            DrawArraysInstancedCommandNV{
                header, object->mesh_renderer().mesh().draw_mode(),
                (unsigned int)object->mesh_renderer().mesh().positions().size(),
                1, 0, 0},
            &token_buffer);
      }

      int size = token_buffer.size() - offset;
      token_sequence.offsets[i] = offset;
      token_sequence.sizes[i] = size;
    }

    // Transfer data to buffer
    if (command_list_data_.command_stream_buffer_size < token_buffer.size()) {
      glNamedBufferData(command_list_data_.command_stream_buffer,
                        token_buffer.size(), token_buffer.c_str(),
                        GL_DYNAMIC_DRAW);
      command_list_data_.command_stream_buffer_size = token_buffer.size();
    } else {
      glNamedBufferSubData(command_list_data_.command_stream_buffer, 0,
                           token_buffer.size(), token_buffer.c_str());
    }
  }
  command_list_data_.draw_commands_compiled = true;

  printf("total captured states: %d\n", state_caches_.size());
}

void CommandListSample::DrawSceneCommandToken() {
  if (!command_list_supported_) {
    return;
  }

  CompileDrawCommandList();
  {
    ProfileTimer timer("  Play draw commands");
    glDisable(GL_LINE_STIPPLE);
    // Play draw commands
    if (!selective_draw_) {
      glDrawCommandsStatesNV(command_list_data_.command_stream_buffer,
                             command_list_data_.token_sequence.offsets.data(),
                             command_list_data_.token_sequence.sizes.data(),
                             command_list_data_.token_sequence.states.data(),
                             command_list_data_.token_sequence.fbos.data(),
                             command_list_data_.token_sequence.offsets.size());
    } else {
      int start =
          glm::clamp<int>(selective_draw_start_, 0,
                          command_list_data_.token_sequence.offsets.size() - 1);
      int end =
          glm::clamp<int>(selective_draw_start_ + selective_draw_count_, start,
                          command_list_data_.token_sequence.offsets.size() - 1);
      glDrawCommandsStatesNV(
          command_list_data_.command_stream_buffer,
          command_list_data_.token_sequence.offsets.data() + start,
          command_list_data_.token_sequence.sizes.data() + start,
          command_list_data_.token_sequence.states.data() + start,
          command_list_data_.token_sequence.fbos.data() + start,
          end - start + 1);
    }
  }
}

void CommandListSample::DrawSceneCommandList() {
  if (!command_list_supported_) {
    return;
  }
}

void CommandListSample::ResizeCommandListRenderbuffers(int w, int h) {
  if (command_list_data_.color_texture) {
    glMakeTextureHandleNonResidentARB(command_list_data_.color_texture_handle);
    glMakeTextureHandleNonResidentARB(
        command_list_data_.depth_stencil_texture_handle);

    glDeleteTextures(1, &command_list_data_.color_texture);
    glDeleteTextures(1, &command_list_data_.depth_stencil_texture);
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
  glMakeTextureHandleResidentARB(command_list_data_.color_texture_handle);
  glMakeTextureHandleResidentARB(
      command_list_data_.depth_stencil_texture_handle);
}

void CommandListSample::InitializeCommandListResouce() {
  glGenBuffers(1, &command_list_data_.command_stream_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, command_list_data_.command_stream_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glGenFramebuffers(1, &command_list_data_.fallback_framebuffer);

  ResizeCommandListRenderbuffers(width, height);
}

void CommandListSample::FinalizeCommandListResouce() {
  glDeleteFramebuffers(1, &command_list_data_.fallback_framebuffer);
  glDeleteTextures(1, &command_list_data_.color_texture);
  glDeleteTextures(1, &command_list_data_.depth_stencil_texture);

  std::vector<GLuint> all_cached_states;
  std::transform(state_caches_.begin(), state_caches_.end(),
                 std::back_inserter(all_cached_states),
                 [](const CaptureStateData& capture_state) {
                   return capture_state.state_object;
                 });

  glDeleteStatesNV(all_cached_states.size(), all_cached_states.data());

  glMakeTextureHandleNonResidentARB(command_list_data_.color_texture_handle);
  glMakeTextureHandleNonResidentARB(
      command_list_data_.depth_stencil_texture_handle);
}

GLuint CommandListSample::CaptureState(const CapturedStateCache& state_cache) {
  auto iter =
      std::find_if(state_caches_.begin(), state_caches_.end(),
                   [&state_cache](const CaptureStateData& capture_state) {
                     return capture_state.state_cached == state_cache;
                   });
  if (iter != state_caches_.end()) {
    return iter->state_object;
  }
  GLuint state_object;
  glCreateStatesNV(1, &state_object);
  state_cache.ApplyState();
  glStateCaptureNV(state_object, state_cache.base_draw_mode);
  state_caches_.push_back(CaptureStateData{state_cache, state_object});
  return state_object;
}

void CommandListSample::CapturedStateCache::ApplyState() const {
  glUseProgram(program);
  if (enable_line_stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(stipple_factor, stipple_pattern);
  } else {
    glDisable(GL_LINE_STIPPLE);
  }

  MeshRenderer::SetupVertexAttribFormat(vertex_attrib_mask);
}

#undef min
#undef max
