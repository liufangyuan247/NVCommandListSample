#include "core/buffer_manager.h"

BufferProxy::~BufferProxy() {
  manager_->ReleaseBuffer(allocator_, buffer_id_, offset_, size_);
}
