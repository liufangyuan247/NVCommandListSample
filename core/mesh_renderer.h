#pragma once

#include <vector>

#include "core/mesh.h"

class MeshRenderer {
 public:
  MeshRenderer() = default;
  ~MeshRenderer() {
    glMakeNamedBufferNonResidentNV(vbo_);
    if (mesh_.indexed_draw()) {
      glMakeNamedBufferNonResidentNV(ibo_);
    }

    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ibo_);    
  }
  void set_mesh(Mesh&& mesh) { mesh_ = mesh; }
  const Mesh& mesh() const { return mesh_; }

  bool initialized() const { return vao_; }
  void Initialize() {
    if (mesh_.positions().empty()) {
      return;
    }
    if (!vao_) {
      glGenVertexArrays(1, &vao_);
    }
    if (!vbo_) {
      glGenBuffers(1, &vbo_);
    }
    if (mesh_.indexed_draw() && !ibo_) {
      glGenBuffers(1, &ibo_);
    }

    glBindVertexArray(vao_);

    element_stride_ = SetupVertexAttribFormat();
    int vertex_count = mesh_.positions().size();
    int total_size = element_stride_ * vertex_count;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, total_size, 0, GL_STATIC_DRAW);

    unsigned char* buffer = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    for (int i = 0; i < vertex_count; ++i) {
      memcpy(buffer, &mesh_.positions()[i], sizeof(Mesh::PositionType));
      buffer += sizeof(Mesh::PositionType);
      if (mesh_.colors().size()) {
        memcpy(buffer, &mesh_.colors()[i], sizeof(Mesh::ColorType));
        buffer += sizeof(Mesh::ColorType);
      }
      if (mesh_.uvs().size()) {
        memcpy(buffer, &mesh_.uvs()[i], sizeof(Mesh::UVType));
        buffer += sizeof(Mesh::UVType);
      }
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindVertexBuffer(0, vbo_, 0, element_stride_);

    glGetNamedBufferParameterui64vNV(vbo_, GL_BUFFER_GPU_ADDRESS_NV, &vbo_address_);
    glMakeNamedBufferResidentNV(vbo_, GL_READ_ONLY);

    if (mesh_.indexed_draw()) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   sizeof(Mesh::IndexType) * mesh_.indices().size(),
                   mesh_.indices().data(), GL_STATIC_DRAW);

      glGetNamedBufferParameterui64vNV(ibo_, GL_BUFFER_GPU_ADDRESS_NV, &ibo_address_);
      glMakeNamedBufferResidentNV(ibo_, GL_READ_ONLY);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  void Render() {
    if (!initialized()) {
      return;
    }
    glBindVertexArray(vao_);
    glBindVertexBuffer(0, vbo_, 0, element_stride_);

    if (mesh_.indexed_draw()) {
      glDrawElements(mesh_.draw_mode(), mesh_.indices().size(), GL_UNSIGNED_INT, 0);
    } else {
      glDrawArrays(mesh_.draw_mode(), 0, mesh_.positions().size());
    }
  }

  int SetupVertexAttribFormat() const {
    return SetupVertexAttribFormat(vertex_attrib_mask());
  }

  static int SetupVertexAttribFormat(uint16_t vertex_attrib_mask) {
    int offset = 0;
    if (vertex_attrib_mask & (1 << POSITION)) {
      glEnableVertexAttribArray(POSITION);
      glVertexAttribFormat(POSITION, Mesh::PositionType::length(), GL_FLOAT,
                          GL_FALSE, offset);
      glVertexAttribBinding(POSITION, 0);
      offset += sizeof(Mesh::PositionType);
    }

    if (vertex_attrib_mask & (1 << COLOR)) {
      glEnableVertexAttribArray(COLOR);
      glVertexAttribFormat(COLOR, Mesh::ColorType::length(), GL_UNSIGNED_BYTE,
                           GL_TRUE, offset);
      glVertexAttribBinding(COLOR, 0);
      offset += sizeof(Mesh::ColorType);
    }
    if (vertex_attrib_mask & (1 << UV)) {
      glEnableVertexAttribArray(UV);
      glVertexAttribFormat(UV, Mesh::ColorType::length(), GL_FLOAT, GL_FALSE,
                           offset);
      glVertexAttribBinding(UV, 0);
      offset += sizeof(Mesh::UVType);
    }
    glBindVertexBuffer(0, 0, 0, offset);
    glVertexBindingDivisor(0, 0);
    return offset;
  }

  uint16_t vertex_attrib_mask() const {
    uint16_t mask = 0;
    if (mesh_.positions().size()) {
      mask |= 1 << POSITION;
    }
    if (mesh_.colors().size()) {
      mask |= 1 << COLOR;
    }
    if (mesh_.uvs().size()) {
      mask |= 1 << UV;
    }
    return mask;
  }

  GLuint64 vbo_address() const {
    return vbo_address_;
  }

  GLuint64 ibo_address() const {
    return ibo_address_;
  }

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ibo_ = 0;
  GLuint64 vbo_address_ = 0;
  GLuint64 ibo_address_ = 0;
  Mesh mesh_;
  int element_stride_ = 0;
};