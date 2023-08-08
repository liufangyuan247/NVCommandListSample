#pragma once

#include <vector>

#include "core/buffer_manager.h"
#include "core/mesh.h"

class MeshRenderer {
 public:
  MeshRenderer() = default;
  ~MeshRenderer() { glDeleteVertexArrays(1, &vao_); }
  void set_mesh(Mesh&& mesh) { mesh_ = mesh; }
  const Mesh& mesh() const { return mesh_; }

  bool initialized() const { return vao_; }
  void Initialize(BufferManager* buffer_manager) {
    if (mesh_.positions().empty()) {
      return;
    }
    if (!vao_) {
      glGenVertexArrays(1, &vao_);
    }

    glBindVertexArray(vao_);
    uint64_t total_size = VertexAttribSize();
    vbo_proxy_ = buffer_manager->AllocateBuffer(total_size);
    if (vbo_proxy_) {
      std::vector<unsigned char> buffer(total_size);
      FillVertexBufferInterleaved(buffer.data());
      vbo_proxy_->SetData(buffer.data(), 0, total_size);
      // void* buffer = vbo_proxy_->Map(GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
      // FillVertexBufferInterleaved(buffer);
      // vbo_proxy_->Unmap();
    }
    SetupVertexAttribFormat();
    glBindVertexBuffer(0, 0, 0, VertexAttribStride());

    if (mesh_.indexed_draw()) {
      ibo_proxy_ = buffer_manager->AllocateBuffer(sizeof(Mesh::IndexType) *
                                                  mesh_.indices().size());

      if (ibo_proxy_) {
        ibo_proxy_->SetData(mesh_.indices().data(), 0,
                            sizeof(Mesh::IndexType) * mesh_.indices().size());
      }
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_proxy_->buffer_id());
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
    glBindVertexBuffer(0, vbo_proxy_->buffer_id(), vbo_proxy_->offset(),
                       VertexAttribStride());

    if (mesh_.indexed_draw()) {
      glDrawElements(mesh_.draw_mode(), mesh_.indices().size(), GL_UNSIGNED_INT,
                     reinterpret_cast<const void*>(ibo_proxy_->offset()));
    } else {
      glDrawArrays(mesh_.draw_mode(), 0, mesh_.positions().size());
    }
  }

  void SetupVertexAttribFormat() const {
    SetupVertexAttribFormat(vertex_attrib_mask());
  }

  static void SetupVertexAttribFormat(uint16_t vertex_attrib_mask) {
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
      glVertexAttribFormat(UV, Mesh::UVType::length(), GL_FLOAT, GL_FALSE,
                           offset);
      glVertexAttribBinding(UV, 0);
      offset += sizeof(Mesh::UVType);
    }
    glBindVertexBuffer(0, 0, 0, offset);
    glVertexBindingDivisor(0, 0);
  }

  uint32_t VertexAttribStride() const {
    int16_t vam = vertex_attrib_mask();
    int stride = 0;
    if (vam & (1 << POSITION)) {
      stride += sizeof(Mesh::PositionType);
    }
    if (vam & (1 << COLOR)) {
      stride += sizeof(Mesh::ColorType);
    }
    if (vam & (1 << UV)) {
      stride += sizeof(Mesh::UVType);
    }
    return stride;
  }

  uint64_t VertexAttribSize() const {
    return VertexAttribStride() * mesh_.positions().size();
  }

  void FillVertexBufferInterleaved(void* buffer) const {
    int16_t vam = vertex_attrib_mask();
    int vertex_count = mesh_.positions().size();
    unsigned char* buffer_ptr = (unsigned char*)buffer;
    for (int i = 0; i < vertex_count; ++i) {
      if (vam & (1 << POSITION)) {
        memcpy(buffer_ptr, &mesh_.positions()[i], sizeof(Mesh::PositionType));
        buffer_ptr += sizeof(Mesh::PositionType);
      }
      if (vam & (1 << COLOR)) {
        memcpy(buffer_ptr, &mesh_.colors()[i], sizeof(Mesh::ColorType));
        buffer_ptr += sizeof(Mesh::ColorType);
      }
      if (vam & (1 << UV)) {
        memcpy(buffer_ptr, &mesh_.uvs()[i], sizeof(Mesh::UVType));
        buffer_ptr += sizeof(Mesh::UVType);
      }
    }
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

  const BufferProxy* vbo() const { return vbo_proxy_.get(); }
  const BufferProxy* ibo() const { return ibo_proxy_.get(); }

 private:
  std::unique_ptr<BufferProxy> vbo_proxy_;
  std::unique_ptr<BufferProxy> ibo_proxy_;
  GLuint vao_ = 0;
  Mesh mesh_;
};