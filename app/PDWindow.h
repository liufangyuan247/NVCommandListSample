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
  void DrawSceneCommandList();

  common::SceneData scene_data_;
  GLuint scene_ubo_;

  common::ObjectData object_data_;
  GLuint object_ubo_;

  enum DrawMethod {
    kBasic = 0,
    kBasicUniformBuffer,
    kCommandList,
    kMethodCount,
  } draw_method_ = kBasic;

  bool command_list_supported_ = false;

  std::vector<std::unique_ptr<RenderObject>> render_objects_;


  Camera camera_;
  ShaderManager shader_manager_;
  nvgl::ProgramManager program_manager_;
};
