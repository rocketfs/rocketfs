// Copyright 2025 RocketFS

#include "namenode/transaction_manager/table/inode_table.h"

#include <absl/base/internal/endian.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <flatbuffers/string.h>
#include <string.h>

#include <optional>
#include <utility>

#include "common/logger.h"
#include "common/status.h"
#include "generated/inode_generated.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

bool operator==(const INodeBasicInfo& lhs, const INodeBasicInfoT& rhs) {
  return lhs.parent_inode_id() == rhs.parent_inode_id &&
         lhs.name()->c_str() == rhs.name && lhs.inode_id() == rhs.inode_id &&
         lhs.created_txid() == rhs.created_txid &&
         lhs.renamed_txid() == rhs.renamed_txid;
}

bool operator==(const INodeTimestamps& lhs, const INodeTimestampsT& rhs) {
  return lhs.ctime_in_nanoseconds() == rhs.ctime_in_nanoseconds &&
         lhs.mtime_in_nanoseconds() == rhs.mtime_in_nanoseconds &&
         lhs.atime_in_nanoseconds() == rhs.atime_in_nanoseconds;
}

INodeTable::INodeTable(
    KVStoreBase* kv_store,
    const std::pmr::polymorphic_allocator<std::byte>& allocator)
    : kv_store_(kv_store), allocator_(allocator) {
  CHECK_NOTNULL(kv_store_);
}

Status INodeTable::Read(SnapshotBase* snapshot,
                        INodeID parent_inode_id,
                        std::string_view name,
                        INode* inode) {
  std::pmr::string inode_basic_info_str(allocator_);
  auto status = kv_store_->Read(snapshot,
                                kINodeBasicInfoCFIndex,
                                EncodeKey(parent_inode_id, name),
                                &inode_basic_info_str);
  if (!status.IsOK()) {
    return status;
  }
  CHECK(!inode_basic_info_str.empty());

  std::pmr::string inode_timestamps_str(allocator_);
  CHECK(kv_store_
            ->Read(snapshot,
                   kINodeTimestampsCFIndex,
                   EncodeKey(parent_inode_id, name),
                   &inode_timestamps_str)
            .IsOK());
  CHECK(!inode_timestamps_str.empty());

  *inode =
      INode(std::move(inode_basic_info_str), std::move(inode_timestamps_str));
  CHECK_EQ(inode->parent_inode_id(), parent_inode_id);
  CHECK_EQ(inode->name(), name);
  return Status::OK();
}

void INodeTable::Write(const std::optional<INode>& original,
                       const INodeT& modified,
                       WriteBatchBase* write_batch) {
  auto write_group = write_batch->AddWriteGroup();
  auto modified_key = EncodeKey(modified.inode_id(), modified.name());
  if (original) {
    auto original_key = EncodeKey(original->inode_id(), original->name());
    if (original_key != modified_key) {
      write_group->Delete(kINodeBasicInfoCFIndex, original_key);
      write_group->Delete(kINodeTimestampsCFIndex, original_key);
    }
  }
  if (!(original && original->basic_info() == modified.basic_info())) {
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(INodeBasicInfo::Pack(fbb, &modified.basic_info()));
    auto value = std::string_view{
        reinterpret_cast<const char*>(fbb.GetBufferPointer()), fbb.GetSize()};
    write_group->Put(kINodeBasicInfoCFIndex, modified_key, value);
  }
  if (!(original && original->timestamps() == modified.timestamps())) {
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(INodeTimestamps::Pack(fbb, &modified.timestamps()));
    auto value = std::string_view{
        reinterpret_cast<const char*>(fbb.GetBufferPointer()), fbb.GetSize()};
    write_group->Put(kINodeTimestampsCFIndex, modified_key, value);
  }
}

std::pmr::string INodeTable::EncodeKey(INodeID inode_id,
                                       std::string_view name) {
  std::pmr::string inode_id_str(allocator_);
  inode_id_str.reserve(sizeof(inode_id) + name.size());
  inode_id_str.resize(sizeof(inode_id));
  absl::big_endian::Store64(inode_id_str.data(), inode_id.index);
  inode_id_str.append(name);
  return inode_id_str;
}

}  // namespace rocketfs
