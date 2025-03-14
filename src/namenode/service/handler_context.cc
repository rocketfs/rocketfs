// Copyright 2025 RocketFS

#include "namenode/service/handler_context.h"

#include <gflags/gflags.h>

#include <memory>
#include <utility>

#include "common/logger.h"
#include "namenode/path_resolver/path_resolver.h"
#include "namenode/table/inode_table.h"

namespace rocketfs {

DECLARE_uint32(request_monotonic_buffer_resource_prealloc_bytes);

HandlerContext::HandlerContext(NameNodeContext* namenode_context)
    : namenode_context_(namenode_context),
      request_monotonic_buffer_resource_prealloc_bytes_(
          FLAGS_request_monotonic_buffer_resource_prealloc_bytes),
      memory_resource_holder_(std::make_unique<std::byte[]>(
          request_monotonic_buffer_resource_prealloc_bytes_)),
      monotonic_buffer_resource_(
          memory_resource_holder_.get(),
          request_monotonic_buffer_resource_prealloc_bytes_),
      allocator_(&monotonic_buffer_resource_),
      transaction_(
          namenode_context_->GetKVStore()->StartTransaction(allocator_)),
      inode_table_(
          std::make_unique<InodeTable>(transaction_.get(), allocator_)),
      path_resolver_(
          std::make_unique<PathResolver>(inode_table_.get(), allocator_)) {
  CHECK_NOTNULL(namenode_context_);
  CHECK_GT(request_monotonic_buffer_resource_prealloc_bytes_, 0);
  CHECK_NOTNULL(memory_resource_holder_);
  CHECK_NOTNULL(transaction_);
  CHECK_NOTNULL(inode_table_);
  CHECK_NOTNULL(path_resolver_);
}

NameNodeContext* HandlerContext::GetNameNodeContext() {
  return namenode_context_;
}

std::unique_ptr<TransactionBase> HandlerContext::GetTransaction() {
  return std::move(transaction_);
}

RequestScopedAllocator HandlerContext::GetAllocator() {
  return allocator_;
}

InodeTableBase* HandlerContext::GetInodeTable() {
  return inode_table_.get();
}

PathResolverBase* HandlerContext::GetPathResolver() {
  return path_resolver_.get();
}

}  // namespace rocketfs
