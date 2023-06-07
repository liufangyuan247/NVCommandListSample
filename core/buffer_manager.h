#pragma once

#include <GL/glew.h>

#include <algorithm>
#include <list>
#include <map>
#include <memory>

class Allocator;
class BufferManager;
class BufferProxy {
 public:
  ~BufferProxy();

  GLuint buffer_id() const { return buffer_id_; }
  int offset() const { return offset_; }
  int size() const { return size_; }

  void SetData(const void* data, int offset, int size) {
    glNamedBufferSubData(buffer_id_, offset_ + offset, size, data);
  }

  void* Map(int access_bits) {
    return glMapNamedBufferRange(buffer_id_, offset_, size_, access_bits);
  }

  void Unmap() { glUnmapNamedBuffer(buffer_id_); };

 private:
  BufferProxy(GLuint buffer_id, int offset, int size)
      : buffer_id_(buffer_id), offset_(offset), size_(size) {}

  GLuint buffer_id_;
  int offset_;
  int size_;
  friend class BufferManager;

  Allocator* allocator_ = nullptr;
  BufferManager* manager_ = nullptr;
};

class Allocator {
 public:
  Allocator(int block_size) { free_ranges_.push_back({0, block_size}); }
  ~Allocator() = default;

  int64_t AllocateBuffer(int size) {
    // Find first range that can fit the size.
    auto iter =
        std::find_if(free_ranges_.begin(), free_ranges_.end(),
                     [size](const Range& range) { return range.size >= size; });

    if (iter == free_ranges_.end()) {
      return -1;
    }

    int64_t offset = iter->offset;
    iter->offset += size;
    iter->size -= size;

    if (iter->size == 0) {
      free_ranges_.erase(iter);
    }

    return offset;
  }

  void ReleaseBuffer(int64_t offset, int size) {
    // Find insert position.
    auto iter = std::find_if(
        free_ranges_.begin(), free_ranges_.end(),
        [offset](const Range& range) { return range.offset > offset; });

    bool should_insert = true;
    // check can merge with previous range.
    if (iter != free_ranges_.begin()) {
      auto prev_iter = std::prev(iter);
      if (prev_iter->offset + prev_iter->size == offset) {
        prev_iter->size += size;
        offset = prev_iter->offset;
        size = prev_iter->size;
        iter = prev_iter;
        should_insert = false;
      }
    }

    // check can merge with next range.
    if (iter != free_ranges_.end()) {
      if (offset + size == iter->offset) {
        iter->offset = offset;
        iter->size += size;
        size = iter->size;
        ++iter;
        should_insert = false;
      }
    }

    if (should_insert) {
      free_ranges_.insert(iter, {offset, size});
    }
  }

 private:
  struct Range {
    int64_t offset;
    int64_t size;
  };

  std::list<Range> free_ranges_;
};

class BufferManager {
 public:
  BufferManager(int block_size) : block_size_(block_size) {}
  ~BufferManager() = default;

  std::unique_ptr<BufferProxy> AllocateBuffer(int size) {
    if (size > block_size_) {
      GLuint buffer_id;
      glCreateBuffers(1, &buffer_id);
      glNamedBufferStorage(buffer_id, size, nullptr,
                           GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
      auto proxy =
          std::unique_ptr<BufferProxy>(new BufferProxy(buffer_id, 0, size));
      proxy->manager_ = this;
      return proxy;
    }

    for (auto& pair : buffers_) {
      auto offset = pair.second->AllocateBuffer(size);
      if (offset != -1) {
        auto proxy = std::unique_ptr<BufferProxy>(
            new BufferProxy(pair.first, offset, size));
        proxy->allocator_ = pair.second.get();
        proxy->manager_ = this;
        return proxy;
      }
    }

    GLuint buffer_id;
    glCreateBuffers(1, &buffer_id);
    glNamedBufferStorage(buffer_id, block_size_, nullptr,
                         GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
    buffers_.emplace(buffer_id, std::make_unique<Allocator>(block_size_));

    return AllocateBuffer(size);
  }

  void ReleaseBuffer(Allocator* allocator, GLuint buffer_id, int offset,
                     int size) {
    if (allocator) {
      allocator->ReleaseBuffer(offset, size);
    } else {
      glDeleteBuffers(1, &buffer_id);
    }
  }

  GLuint64 GetBufferAddress(GLuint buffer_id) {
    if (buffers_address_.find(buffer_id) != buffers_address_.end()) {
      return buffers_address_[buffer_id];
    }
    GLuint64 address;
    glGetNamedBufferParameterui64vNV(buffer_id, GL_BUFFER_GPU_ADDRESS_NV,
                                     &address);
    glMakeNamedBufferResidentNV(buffer_id, GL_READ_ONLY);
    buffers_address_[buffer_id] = address;
    return address;
  }

 private:
  const int block_size_;
  std::map<GLuint, GLuint64> buffers_address_;
  std::map<GLuint, std::unique_ptr<Allocator>> buffers_;
};
