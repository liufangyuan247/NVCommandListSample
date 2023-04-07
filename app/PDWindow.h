#pragma once
#include <map>
#include <memory>
#include <random>
#include <stack>
#include <thread>
#include <vector>

#include <glm/glm.hpp>

#include "core/Texture2D.h"
#include "core/Window.h"

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
  void DrawTriangleBasic();
  void DrawTriangleCommandList();

  struct SceneData {
    glm::mat4 VP;
  } scene_data_;
  GLuint scene_ubo_;

  struct ObjectData {
    glm::mat4 M;
  } object_data_;
  GLuint object_ubo_;

  enum DrawMethod {
    kBasic,
    kCommandList,
  } method_ = kBasic;

  bool command_list_supported_ = false;

  GLuint vao_;
  GLuint vbo_;

  GLuint normal_shader_;
  GLuint command_list_shader_;
};
