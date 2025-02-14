// Copyright 2025 RocketFS

#include "namenode/transaction_manager/table/inode_table.h"

#include <absl/base/internal/endian.h>

#include <utility>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

Status INodeTable::Read(SnapshotBase* snapshot,
                        INodeID parent_inode_id,
                        std::string_view name,
                        INode* inode) {
  std::pmr::string inode_basic_info_str(memory_resource_);
  auto status = kv_store_->Read(snapshot,
                                kINodeBasicInfoCFIndex,
                                EncodeKey(parent_inode_id, name),
                                &inode_basic_info_str);
  if (!status.IsOK()) {
    return status;
  }
  CHECK(!inode_basic_info_str.empty());

  std::pmr::string inode_timestamps_str(memory_resource_);
  CHECK(kv_store_
            ->Read(snapshot,
                   kINodeTimestampsCFIndex,
                   EncodeKey(parent_inode_id, name),
                   &inode_timestamps_str)
            .IsOK());
  CHECK(!inode_timestamps_str.empty());

  *inode =
      INode(std::move(inode_basic_info_str), std::move(inode_timestamps_str));
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
