// Copyright 2025 RocketFS

#pragma once

#include <memory_resource>
#include <string>
#include <string_view>

#include "namenode/kv_store/kv_store_base.h"
#include "namenode/transaction_manager/table/inode_table_base.h"

namespace rocketfs {

class INodeTable : public INodeTableBase {
 public:
  INodeTable() = default;
  INodeTable(const INodeTable&) = delete;
  INodeTable(INodeTable&&) = delete;
  INodeTable& operator=(const INodeTable&) = delete;
  INodeTable& operator=(INodeTable&&) = delete;
  ~INodeTable() = default;

  Status Read(SnapshotBase* snapshot,
              INodeID parent_inode_id,
              std::string_view name,
              INode* inode) override;

 private:
  std::pmr::string EncodeKey(INodeID inode_id, std::string_view name);

 private:
  std::pmr::memory_resource* memory_resource_;
  KVStoreBase* kv_store_;
};

}  // namespace rocketfs
