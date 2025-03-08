// Copyright 2025 RocketFS

#include "namenode/service/handler_context.h"

#include <gflags/gflags.h>

#include "common/logger.h"
#include "namenode/transaction_manager/table/inode_table.h"

namespace rocketfs {

DECLARE_uint32(request_monotonic_buffer_resource_prealloc_bytes);

HandlerContext::HandlerContext(NameNodeContext* namenode_context)
    : namenode_context_(namenode_context),
      snapshot_(namenode_context_->GetKVStore()->GetSnapshot()),
      request_monotonic_buffer_resource_prealloc_bytes_(
          FLAGS_request_monotonic_buffer_resource_prealloc_bytes),
      memory_resource_holder_(std::make_unique<std::byte[]>(
          request_monotonic_buffer_resource_prealloc_bytes_)),
      monotonic_buffer_resource_(
          memory_resource_holder_.get(),
          request_monotonic_buffer_resource_prealloc_bytes_),
      allocator_(&monotonic_buffer_resource_),
      inode_table_(std::make_unique<INodeTable>(namenode_context_->GetKVStore(),
                                                allocator_)) {
  CHECK_NOTNULL(namenode_context_);
  CHECK_NOTNULL(snapshot_);
  CHECK_GT(request_monotonic_buffer_resource_prealloc_bytes_, 0);
  CHECK_NOTNULL(memory_resource_holder_);
}

NameNodeContext* HandlerContext::GetNameNodeContext() {
  return namenode_context_;
}

SnapshotBase* HandlerContext::GetSnapshot() {
  return snapshot_.get();
}

std::pmr::polymorphic_allocator<std::byte> HandlerContext::GetAllocator() {
  return allocator_;
}

INodeTableBase* HandlerContext::GetINodeTable() {
  return inode_table_.get();
}

}  // namespace rocketfs
