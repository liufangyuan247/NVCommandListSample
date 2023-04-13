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

class PDWindow : public Window {
 public:
  PDWindow();
  ~PDWindow();
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

  common::SceneData scene_data_;
  GLuint scene_ubo_;

  GLuint object_ubo_;
  int object_ubo_size_= 0;

  enum DrawMethod {
    kBasic = 0,
    kBasicUniformBuffer,
    kCommandToken,
    kCommandList,
    kMethodCount,
  } draw_method_ = kBasicUniformBuffer;

  struct NVTokenSequence {
    std::vector<GLintptr> offsets;
    std::vector<GLsizei> sizes;
    std::vector<GLuint> states;
    std::vector<GLuint> fbos;
  };

  struct CommandListExtensionData {
    // resizes
    GLuint fallback_framebuffer = 0;
    GLuint original_framebuffer = 0;

    GLuint color_texture = 0;
    GLuint64 color_texture_handle = 0;

    GLuint depth_stencil_texture = 0;
    GLuint64 depth_stencil_texture_handle = 0;

    std::vector<GLuint> all_states;

    GLuint command_stream_buffer = 0;
    NVTokenSequence token_sequence;
    NVTokenSequence token_sequence_address;
  } command_list_data_;

  bool command_list_supported_ = false;

  bool roaming_ = false;

  OpenGLContext gl_context_;
  std::vector<std::unique_ptr<RenderObject>> render_objects_;

  Camera camera_;
  ShaderManager shader_manager_;
  nvgl::ProgramManager program_manager_;
};
