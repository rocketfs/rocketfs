// Copyright 2025 RocketFS

#pragma once

#include <string>
#include <string_view>

#include "namenode/common/request_scoped_allocator.h"
#include "namenode/kv_store/kv_store_base.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

class InodeTable : public InodeTableBase {
 public:
  InodeTable(TransactionBase* transaction, RequestScopedAllocator allocator);
  InodeTable(const InodeTable&) = delete;
  InodeTable(InodeTable&&) = delete;
  InodeTable& operator=(const InodeTable&) = delete;
  InodeTable& operator=(InodeTable&&) = delete;
  ~InodeTable() = default;

  std::expected<std::optional<Inode>, Status> Read(InodeID inode_id) override;
  void Write(const ResolvedPaths& resolved_paths) override;

 private:
  std::pmr::string EncodeKey(InodeID inode_id);

 private:
  TransactionBase* transaction_;
  RequestScopedAllocator allocator_;
};

}  // namespace rocketfs
