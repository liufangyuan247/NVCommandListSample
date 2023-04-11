#pragma once

#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

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

 private:
  std::vector<PositionType> positions_;
  std::vector<ColorType> colors_;
  std::vector<UVType> uvs_;
  std::vectorIndexType> indices_;
  GLenum draw_mode_ = GL_TRIANGLES;

};