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

    // Build interleaved buffer data
    struct VertexAttribData {
      int attrib_location;
      GLenum attrib_type;
      int component_size;
      GLenum normalized;
      int element_offset;
      int element_size;
      const void* source;
    };

    int current_offset = 0;
    std::vector<VertexAttribData> vertex_attribs;
    vertex_attribs.push_back(
        VertexAttribData{.attrib_location = POSITION,
                         .attrib_type = GL_FLOAT,
                         .component_size = 3,
                         .normalized = GL_FALSE,
                         .element_offset = current_offset,
                         .element_size = sizeof(Mesh::PositionType),
                         .source = mesh_.positions().data()});

    if (mesh_.colors().size()) {
      current_offset += vertex_attribs.back().element_size;
      vertex_attribs.push_back(
          VertexAttribData{.attrib_location = COLOR,
                           .attrib_type = GL_UNSIGNED_BYTE,
                           .component_size = 4,
                           .normalized = GL_TRUE,
                           .element_offset = current_offset,
                           .element_size = sizeof(Mesh::ColorType),
                           .source = mesh_.colors().data()});
    }

    if (mesh_.uvs().size()) {
      current_offset += vertex_attribs.back().element_size;
      vertex_attribs.push_back(
          VertexAttribData{.attrib_location = UV,
                           .attrib_type = GL_FLOAT,
                           .component_size = 2,
                           .normalized = GL_FALSE,
                           .element_offset = current_offset,
                           .element_size = sizeof(Mesh::UVType),
                           .source = mesh_.uvs().data()});
    }
    current_offset += vertex_attribs.back().element_size;

    int vertex_count = mesh_.positions().size();
    int stride = current_offset;
    int total_size = stride * vertex_count;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, total_size, 0, GL_STATIC_DRAW);

    void *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    for (const auto& attrib_data : vertex_attribs) {
      // Copy data
      for (int i = 0; i < vertex_count; ++i) {
        void* dst_ptr = buffer + (i * stride) + attrib_data.element_offset;
        const void* src_ptr = attrib_data.source + i * attrib_data.element_size;
        memcpy(dst_ptr, src_ptr, attrib_data.element_size);
      }

      // Set up attrib format
      glEnableVertexAttribArray(attrib_data.attrib_location);
      glVertexAttribFormat(attrib_data.attrib_location,
                           attrib_data.component_size, attrib_data.attrib_type,
                           attrib_data.normalized, attrib_data.element_offset);
      glVertexAttribBinding(attrib_data.attrib_location, 0);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindVertexBuffer(0, vbo_, 0, stride);
    glVertexBindingDivisor(0, 0);

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