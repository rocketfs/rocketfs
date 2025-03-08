#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>

#include "namenode/kv_store/kv_store_base.h"
#include "namenode/namenode_context.h"
#include "namenode/transaction_manager/table/inode_table_base.h"

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
  SnapshotBase* GetSnapshot();
  std::pmr::polymorphic_allocator<std::byte> GetAllocator();
  INodeTableBase* GetINodeTable();

 private:
  NameNodeContext* namenode_context_;
  std::unique_ptr<SnapshotBase> snapshot_;
  uint32_t request_monotonic_buffer_resource_prealloc_bytes_;
  std::unique_ptr<std::byte[]> memory_resource_holder_;
  std::pmr::polymorphic_allocator<std::byte> allocator_;
  std::pmr::monotonic_buffer_resource monotonic_buffer_resource_;
  std::unique_ptr<INodeTableBase> inode_table_;
};

}  // namespace rocketfs
