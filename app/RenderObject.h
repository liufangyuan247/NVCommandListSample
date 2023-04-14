#pragma once

#include <glm/gtc/type_ptr.hpp>

#include "app/extension_command_list.h"
#include "app/json.hpp"
#include "core/mesh_renderer.h"
#include "core/shader_manager.h"

class RenderObject;
std::unique_ptr<RenderObject> CreateRenderObjectFromJson(
    const nlohmann::json& json);

template <typename MatType>
inline MatType MatFromJson(const nlohmann::json& json) {
  MatType mat;
  for (int i = 0; i < mat.length(); ++i) {
    mat[i] = VecFromJson<typename MatType::col_type>(json[i]);
  }
  return mat;
}

class RenderObject {
 public:
  using PreRenderCallback = std::function<bool(RenderObject*)>;
  using PostRenderCallback = std::function<void(RenderObject*)>;

  virtual ~RenderObject() = default;

  virtual void SerializeFromJson(const nlohmann::json& json) {}
  virtual void Initialize() {}
  virtual void Render(const ShaderManager& shader_manager,
                            PreRenderCallback pre_render = nullptr,
                            PostRenderCallback post_render = nullptr) {}

  void set_world(const glm::mat4& world) { world_ = world; }
  const glm::mat4& world() const { return world_; }

  void set_shader(const std::string& shader) { shader_ = shader; }
  const std::string& shader() const { return shader_; }

  const MeshRenderer& mesh_renderer() const {
    return mesh_renderer_;
  }

 protected:
  MeshRenderer mesh_renderer_; 

 private:
  std::string shader_;
  glm::mat4 world_;
};

struct LineStyle {
  float line_width = 1.0f;
  bool line_stipple = false;
  int line_stipple_factor = 1;
  int line_stipple_pattern = 0x00FF;

  LineStyle() = default;
  explicit LineStyle(float width) : line_width(width) {}
  LineStyle(float width, int factor, int pattern)
      : line_width(width),
        line_stipple(true),
        line_stipple_factor(factor),
        line_stipple_pattern(pattern) {}

  void SerializeFromJson(const nlohmann::json& json) {
    line_width = json["line_width"];
    line_stipple = json["line_stipple"];
    line_stipple_factor = json["line_stipple_factor"];
    line_stipple_pattern = json["line_stipple_pattern"];
  }
};

class LineObject : public RenderObject {
 public:
  LineObject() = default;

  void SerializeFromJson(const nlohmann::json& json) override {
    const auto& draw_info = json["draw_info"];
    const auto& mesh_json = json["mesh"];
    set_color(VecFromJson<glm::vec4>(draw_info["color"]));
    LineStyle style;
    style.SerializeFromJson(draw_info["line_style"]);
    set_line_style(style);

    set_shader(draw_info["shader"]);
    set_world(MatFromJson<glm::mat4>(draw_info["world_matrix"]));

    Mesh mesh = Mesh::SerializeFromJson(mesh_json);
    mesh.set_draw_mode(draw_info["draw_mode"]);

    mesh_renderer_.set_mesh(std::move(mesh));
  }

  void Initialize() override { mesh_renderer_.Initialize(); }

  void Render(const ShaderManager& shader_manager,
                    PreRenderCallback pre_render = nullptr,
                    PostRenderCallback post_render = nullptr) override {
    if (pre_render && !pre_render(this)) {
      return;
    }
    glLineWidth(line_style_.line_width);
    if (line_style_.line_stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(line_style_.line_stipple_factor,
                    line_style_.line_stipple_pattern);
    }

    mesh_renderer_.Render();

    if (line_style_.line_stipple) {
      glDisable(GL_LINE_STIPPLE);
    }

    if (post_render) {
      post_render(this);
    }
  }

  void set_line_style(const LineStyle& line_style) { line_style_ = line_style; }
  const LineStyle& line_style() const { return line_style_; }

  void set_color(const glm::vec4& color) { color_ = color; }
  const glm::vec4& color() const { return color_; }

 private:
  LineStyle line_style_;
  glm::vec4 color_;
};

class DashedStripeObject : public RenderObject {
 public:
  DashedStripeObject() = default;

  void SerializeFromJson(const nlohmann::json& json) override {
    const auto& draw_info = json["draw_info"];
    const auto& mesh_json = json["mesh"];
    set_color(VecFromJson<glm::vec4>(draw_info["color"]));

    set_shader(draw_info["shader"]);
    set_world(MatFromJson<glm::mat4>(draw_info["world_matrix"]));

    Mesh mesh = Mesh::SerializeFromJson(mesh_json);
    mesh.set_draw_mode(draw_info["draw_mode"]);

    mesh_renderer_.set_mesh(std::move(mesh));
  }

  void Initialize() override { mesh_renderer_.Initialize(); }

  void Render(const ShaderManager& shader_manager,
                    PreRenderCallback pre_render = nullptr,
                    PostRenderCallback post_render = nullptr) override {
    if (pre_render && !pre_render(this)) {
      return;
    }
    mesh_renderer_.Render();
    if (post_render) {
      post_render(this);
    }
  }

  void set_color(const glm::vec4& color) { color_ = color; }
  const glm::vec4& color() const { return color_; }

 private:
  glm::vec4 color_;
};

class SimpleTexturedObject : public RenderObject {
 public:
  SimpleTexturedObject() = default;

  void SerializeFromJson(const nlohmann::json& json) override {
    const auto& draw_info = json["draw_info"];
    const auto& mesh_json = json["mesh"];
    set_alpha(draw_info["alpha"]);

    set_shader(draw_info["shader"]);
    set_world(MatFromJson<glm::mat4>(draw_info["world_matrix"]));

    Mesh mesh = Mesh::SerializeFromJson(mesh_json);
    mesh.set_draw_mode(draw_info["draw_mode"]);

    mesh_renderer_.set_mesh(std::move(mesh));
  }

  void Initialize() override { mesh_renderer_.Initialize(); }

  void Render(const ShaderManager& shader_manager,
                    PreRenderCallback pre_render = nullptr,
                    PostRenderCallback post_render = nullptr) override {
    if (pre_render && !pre_render(this)) {
      return;
    }
    mesh_renderer_.Render();
    if (post_render) {
      post_render(this);
    }
  }

  void set_alpha(float alpha) { alpha_ = alpha; }
  float alpha() const { return alpha_; }

 private:
  float alpha_;
};

class RoadElementObject : public RenderObject {
 public:
  RoadElementObject() = default;

  void SerializeFromJson(const nlohmann::json& json) override {
    const auto& sub_meshes = json["sub_mesh"];

    for (auto& sub_mesh_json : sub_meshes) {
      auto mesh_render = CreateRenderObjectFromJson(sub_mesh_json);
      if (mesh_render) {
        sub_meshes_.emplace_back(std::move(mesh_render));
      }
    }
  }

  void Initialize() override {
    for (auto& sub_mesh : sub_meshes_) {
      sub_mesh->Initialize();
    }
  }

  void Render(const ShaderManager& shader_manager,
                    PreRenderCallback pre_render = nullptr,
                    PostRenderCallback post_render = nullptr) override {
    if (pre_render && !pre_render(this)) {
      return;
    }
    for (auto& sub_mesh : sub_meshes_) {
      sub_mesh->Render(shader_manager, pre_render, post_render);
    }
    if (post_render) {
      post_render(this);
    }
  }

 private:
  std::vector<std::unique_ptr<RenderObject>> sub_meshes_;
};