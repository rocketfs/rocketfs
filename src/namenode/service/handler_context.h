#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>

#include "namenode/common/request_scoped_allocator.h"
#include "namenode/kv_store/kv_store_base.h"
#include "namenode/namenode_context.h"
#include "namenode/path_resolver/path_resolver_base.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

class HandlerContext {
 public:
  explicit HandlerContext(NameNodeContext* namenode_context);
  HandlerContext(const HandlerContext&) = delete;
  HandlerContext(HandlerContext&&) = delete;
  HandlerContext& operator=(const HandlerContext&) = delete;
  HandlerContext& operator=(HandlerContext&&) = delete;
  ~HandlerContext() = default;

  NameNodeContext* GetNameNodeContext();
  std::unique_ptr<TransactionBase> GetTransaction();
  RequestScopedAllocator GetAllocator();
  InodeTableBase* GetInodeTable();
  PathResolverBase* GetPathResolver();

 private:
  NameNodeContext* namenode_context_;

  uint32_t request_monotonic_buffer_resource_prealloc_bytes_;
  std::unique_ptr<std::byte[]> memory_resource_holder_;
  std::pmr::monotonic_buffer_resource monotonic_buffer_resource_;
  RequestScopedAllocator allocator_;

  std::unique_ptr<TransactionBase> transaction_;
  std::unique_ptr<InodeTableBase> inode_table_;
  std::unique_ptr<PathResolverBase> path_resolver_;
};

}  // namespace rocketfs
