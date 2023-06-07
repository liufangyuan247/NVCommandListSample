#include "core/buffer_manager.h"

#include <assert.h>

BufferProxy::~BufferProxy() {
  manager_->ReleaseBuffer(allocator_, buffer_id_, offset_, size_);
}

const int32_t BufferAllocator::kInvalidOffset;

BufferAllocator::BufferAllocator(int32_t capacity) : capacity_(capacity) {
  assert(capacity_ > 0);
}

FirstFitBufferAllocator::FirstFitBufferAllocator(int32_t capacity) : BufferAllocator(capacity) {
  free_ranges_.push_back({0, capacity});
  skip_list_.insert({0, free_ranges_.begin()});
}

int32_t FirstFitBufferAllocator::Alloc(int32_t size) {
  // Find first range that can fit the size.
  auto iter = std::find_if(free_ranges_.begin(), free_ranges_.end(), [size](const Range& range) {
    return range.size >= size;
  });

  if (iter == free_ranges_.end()) {
    return kInvalidOffset;
  }

  Range range = *iter;
  iter = free_ranges_.erase(iter);
  skip_list_.erase(range.offset);

  int32_t offset = range.offset;
  range.offset += size;
  range.size -= size;

  if (range.size > 0) {
    iter = free_ranges_.insert(iter, range);
    skip_list_[range.offset] = iter;
  }

  return offset;
}

void FirstFitBufferAllocator::Free(int32_t offset, int32_t size) {
  // Find insert position.
  std::list<Range>::iterator iter = free_ranges_.end();
  auto skip_iter = skip_list_.lower_bound(offset);
  if (skip_iter != skip_list_.end()) {
    iter = skip_iter->second;
  }

  Range range = {offset, size};

  if (iter != free_ranges_.begin()) {
    auto prev_iter = std::prev(iter);
    if (prev_iter->offset + prev_iter->size == range.offset) {
      range.offset = prev_iter->offset;
      range.size += prev_iter->size;
      free_ranges_.erase(prev_iter);
      skip_list_.erase(range.offset);
    }
  }

  if (iter != free_ranges_.end()) {
    if (range.offset + range.size == iter->offset) {
      range.size += iter->size;
      skip_list_.erase(iter->offset);
      iter = free_ranges_.erase(iter);
    }
  }

  iter = free_ranges_.insert(iter, range);
  skip_list_[range.offset] = iter;
}
