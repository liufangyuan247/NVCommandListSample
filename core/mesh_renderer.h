#pragma once

#include <vector>

#include "core/mesh.h"

class MeshRenderer {
 public:
  MeshRenderer() = default;
  ~MeshRenderer() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ibo_);    
  }
  void set_mesh(Mesh&& mesh) { mesh_ = mesh; }

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

    int position_size = mesh_.positions().size() * sizeof(Mesh::PositionType);
    int color_size = mesh_.colors().size() * sizeof(Mesh::ColorType);
    int uv_size = mesh_.uvs().size() * sizeof(Mesh::UVType);

    int total_size = position_size + color_size + uv_size;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, total_size, 0, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    position_size,
                    mesh_.positions().data());
    glEnableVertexAttribArray(Mesh::POSITION);
    // glVertexAttribFormat(Mesh::POSITION, 3, GL_FLOAT, GL_FALSE, 0);
    // glVertexAttribBinding(Mesh::POSITION,0);
    // glBindVertexBuffer(0, vbo_, 0, 0);
    glVertexAttribPointer(Mesh::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (mesh_.colors().size()) {
      glBufferSubData(GL_ARRAY_BUFFER, position_size,
                      color_size,
                      mesh_.colors().data());
      glEnableVertexAttribArray(Mesh::COLOR);
      // glVertexAttribFormat(Mesh::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0);
      // glVertexAttribBinding(Mesh::COLOR, Mesh::COLOR);
      // glBindVertexBuffer(Mesh::COLOR, vbo_, position_size, 0);
      glVertexAttribPointer(Mesh::COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0,
                            (void *)(position_size));
    }

    if (mesh_.uvs().size()) {
      glBufferSubData(GL_ARRAY_BUFFER, position_size + color_size,
                      uv_size,
                      mesh_.uvs().data());
      glEnableVertexAttribArray(Mesh::UV);
      // glVertexAttribFormat(Mesh::UV, 2, GL_FLOAT, GL_FALSE, 0);
      // glVertexAttribBinding(Mesh::UV, Mesh::UV);
      // glBindVertexBuffer(Mesh::UV, vbo_, position_size + color_size, 0);
      glVertexAttribPointer(Mesh::UV, 2, GL_FLOAT, GL_FALSE, 0,
                            (void *)(position_size + color_size));
    }

    if (mesh_.indexed_draw()) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   mesh_.indices().size() * sizeof(Mesh::IndexType),
                   mesh_.indices().data(), GL_STATIC_DRAW);
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
    if (mesh_.indexed_draw()) {
      glDrawElements(mesh_.draw_mode(), mesh_.indices().size(), GL_UNSIGNED_INT, 0);
    } else {
      glDrawArrays(mesh_.draw_mode(), 0, mesh_.positions().size());
    }
  }

 private:
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ibo_ = 0;
  Mesh mesh_;
};