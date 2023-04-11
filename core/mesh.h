#pragma once

#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "app/json.hpp"
#include "app/base64.h"

template <typename VecType>
inline VecType VecFromJson(const nlohmann::json& json) {
  VecType vec;
  for (int i = 0; i < vec.length(); ++i) {
    vec[i] = json[i];
  }
  return vec;
}

class Mesh {
 public:
  enum AttribIndex {
    POSITION,
    COLOR,
    UV
  };

  using PositionType = glm::vec3;
  using ColorType = glm::u8vec4;
  using UVType = glm::vec2;
  using IndexType = unsigned int;

  Mesh() = default;

  const std::vector<PositionType>& positions() const { return positions_; }
  void set_positions(const std::vector<PositionType>& positions) {
    positions_ = positions;
  }

  const std::vector<ColorType>& colors() const { return colors_; }
  void set_colors(const std::vector<ColorType>& colors) { colors_ = colors; }

  const std::vector<UVType>& uvs() const { return uvs_; }
  void set_uvs(const std::vector<UVType>& uvs) { uvs_ = uvs; }

  const std::vector<IndexType> indices() const { return indices_; }
  void set_positions(const std::vector<IndexType>& indices) {
    indices_ = indices;
  }

  bool indexed_draw() const {
    return indices_.size();
  }

  GLenum draw_mode() const { return draw_mode_; }
  void set_draw_mode(GLenum draw_mode) { draw_mode_ = draw_mode; }

  static Mesh SerializeFromJson(const nlohmann::json& mesh_json) {
    Mesh mesh;
    std::vector<PositionType> positions;
    std::string position_data = base64_decode(mesh_json["position"]);
    int position_count = position_data.size() / sizeof(PositionType);
    positions.resize(position_count);
    memcpy(positions.data(), position_data.data(), position_data.size());
    mesh.set_positions(positions);

    if (mesh_json.find("uv") != mesh_json.end()) {
      std::vector<UVType> uvs;
      std::string uv_data = base64_decode(mesh_json["uv"]);
      int uv_count = uv_data.size() / sizeof(UVType);
      uvs.resize(uv_count);
      memcpy(uvs.data(), uv_data.data(), uv_data.size());
      mesh.set_uvs(uvs);
    }
    if (mesh_json.find("color") != mesh_json.end()) {
      std::vector<ColorType> colors;
      std::string color_data = base64_decode(mesh_json["color"]);
      int color_count = color_data.size() / sizeof(ColorType);
      colors.resize(color_count);
      memcpy(colors.data(), color_data.data(), color_data.size());
      mesh.set_colors(colors);
    }

    return mesh;
  }

 private:
  std::vector<PositionType> positions_;
  std::vector<ColorType> colors_;
  std::vector<UVType> uvs_;
  std::vector<IndexType> indices_;
  GLenum draw_mode_ = GL_TRIANGLES;
};