// Copyright 2025 RocketFS

#include "namenode/transaction_manager/table/inode_table.h"

#include "absl/base/internal/endian.h"
#include "common/logger.h"
#include "generated/inode_generated.h"

namespace rocketfs {

Status INodeTable::Read(SnapshotBase* snapshot,
                        INodeID parent_inode_id,
                        std::string_view name,
                        INode* inode) {
  std::pmr::string inode_str(memory_resource_);
  auto status = kv_store_->Read(
      snapshot, kINodeCFIndex, EncodeKey(parent_inode_id, name), &inode_str);
  if (!status.IsOK()) {
    return status;
  }
  CHECK(!inode_str.empty());
  *inode = INode(std::move(inode_str), std::pmr::string());
  CHECK_EQ(inode->parent_inode_id(), parent_inode_id.index);
  CHECK_EQ(inode->name(), name);
  return Status::OK();
}

std::pmr::string INodeTable::EncodeKey(INodeID inode_id,
                                       std::string_view name) {
  std::pmr::string inode_id_str(memory_resource_);
  inode_id_str.reserve(sizeof(inode_id) + name.size());
  inode_id_str.resize(sizeof(inode_id));
  absl::big_endian::Store64(inode_id_str.data(), inode_id.index);
  inode_id_str.append(name);
  return inode_id_str;
}

}  // namespace rocketfs
