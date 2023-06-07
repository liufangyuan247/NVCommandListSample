#pragma once

#include <GL/glew.h>

#include <algorithm>
#include <list>
#include <map>
#include <memory>

class BufferAllocator ;
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

  BufferAllocator* allocator_ = nullptr;
  BufferManager* manager_ = nullptr;
};

class BufferAllocator {
 public:
  static const int32_t kInvalidOffset = -1;
  explicit BufferAllocator(int32_t capacity);
  virtual ~BufferAllocator() = default;

  // Returns an offset in the buffer if allocation succeeds, otherwise returns kInvalidOffset.
  virtual int32_t Alloc(int32_t size) = 0;
  // Frees the buffer usage at the given offset.
  virtual void Free(int32_t offset, int32_t size) = 0;

  int32_t capacity() const { return capacity_; }

 private:
  const int32_t capacity_;
};

class FirstFitBufferAllocator : public BufferAllocator {
 public:
  explicit FirstFitBufferAllocator(int32_t capacity);
  ~FirstFitBufferAllocator() override = default;

  int32_t Alloc(int32_t size) override;
  void Free(int32_t offset, int32_t size) override;

 private:
  struct Range {
    int32_t offset = 0;
    int32_t size = 0;
  };

  std::list<Range> free_ranges_;
  std::map<int32_t, std::list<Range>::iterator> skip_list_;
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
      auto offset = pair.second->Alloc(size);
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
    buffers_.emplace(buffer_id, std::make_unique<FirstFitBufferAllocator>(block_size_));

    return AllocateBuffer(size);
  }

  void ReleaseBuffer(BufferAllocator* allocator, GLuint buffer_id, int offset,
                     int size) {
    if (allocator) {
      allocator->Free(offset, size);
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
  std::map<GLuint, std::unique_ptr<BufferAllocator>> buffers_;
};
