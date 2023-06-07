#pragma once
#include <map>
#include <memory>
#include <random>
#include <stack>
#include <thread>
#include <vector>

#include <glm/glm.hpp>

#include "nvgl/programmanager_gl.hpp"

#include "app/common.h"
#include "app/RenderObject.h"
#include "core/Texture2D.h"
#include "core/Window.h"
#include "core/camera.h"
#include "core/mesh_renderer.h"
#include "core/shader_manager.h"
#include "core/opengl_context.h"

class CommandListSample : public Window {
 public:
  CommandListSample();
  ~CommandListSample();
  virtual void onInitialize();
  virtual void onRender();
  virtual void onUIUpdate();
  virtual void onUpdate();
  virtual void onResize(int w, int h);
  virtual void onEndFrame();

 private:
  void DrawSceneBasic();
  void DrawSceneBasicUniformBuffer();
  void DrawSceneCommandToken();
  void DrawSceneCommandList();
  
  void InitializeCommandListResouce();
  void FinalizeCommandListResouce();
  void ResizeCommandListRenderbuffers(int w, int h);

  void BindFallbackFramebuffer();
  void BlitFallbackFramebuffer();

  struct CapturedStateCache;
  GLuint CaptureState(const CapturedStateCache& state_cache);

  void CompileDrawCommandList();
  void CollectRenderObjectData(
      std::vector<common::ObjectData>& object_datas,
      std::vector<RenderObject*>& real_render_objects,
      std::vector<CapturedStateCache>& render_object_states);

  common::SceneData scene_data_;
  GLuint scene_ubo_;
  GLuint64 scene_ubo_address_;

  common::MaterialData material_data_[2];
  GLuint material_ubo_;
  GLuint64 material_ubo_address_;

  GLuint object_ubo_;
  GLuint64 object_ubo_address_;
  int object_ubo_size_= 0;

  GLuint texture_[2];
  GLuint64 texture_address_[2];

  enum DrawMethod {
    kBasic = 0,
    kBasicUniformBuffer,
    kCommandToken,
    kCommandList,
    kMethodCount,
  } draw_method_ = kCommandToken;

  struct NVTokenSequence {
    std::vector<GLintptr> offsets;
    std::vector<GLsizei> sizes;
    std::vector<GLuint> states;
    std::vector<GLuint> fbos;
  };

#pragma pack(push, 1)
  struct CapturedStateCache {
    GLenum base_draw_mode = 0;
    GLuint program = 0;
    uint8_t enable_line_stipple = 0;
    GLint stipple_factor = 1;
    GLushort stipple_pattern = 0xffff;
    uint16_t vertex_attrib_mask = 0;

    bool operator==(const CapturedStateCache& other) const {
      // bit wise compare
      return memcmp(this, &other, sizeof(CapturedStateCache)) == 0;
    }
    bool operator!=(const CapturedStateCache& other) const {
      return !(*this == other);
    }

    void ApplyState() const;
  };

#pragma pack(pop)

  struct CaptureStateData {
    CapturedStateCache state_cached;
    GLuint state_object;
  };

  struct CommandListExtensionData {
    // resizes
    GLuint fallback_framebuffer = 0;
    GLuint original_framebuffer = 0;

    GLuint color_texture = 0;
    GLuint64 color_texture_handle = 0;
    int fbo_width;
    int fbo_height;

    GLuint depth_stencil_texture = 0;
    GLuint64 depth_stencil_texture_handle = 0;

    bool draw_commands_compiled = false;
    GLuint command_stream_buffer = 0;
    uint64_t command_stream_buffer_size = 0;
    std::string command_stream_buffer_cpu_;
    NVTokenSequence token_sequence;
    NVTokenSequence token_sequence_address;

    GLuint command_list_;
  } command_list_data_;

  bool command_list_supported_ = false;

  bool roaming_ = false;
  bool selective_draw_ = false;
  int selective_draw_start_ = 0;
  int selective_draw_count_ = 0;
  float camera_speed_ = 100.0f;

  std::unique_ptr<BufferManager> buffer_manager_;
  OpenGLContext gl_context_;
  std::vector<std::unique_ptr<RenderObject>> render_objects_;

  std::vector<CaptureStateData> state_caches_;

  Camera camera_;
  ShaderManager shader_manager_;
  nvgl::ProgramManager program_manager_;
};
