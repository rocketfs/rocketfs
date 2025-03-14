// Copyright 2025 RocketFS

#include "namenode/table/inode_table.h"

#include <absl/base/internal/endian.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <flatbuffers/string.h>

#include <cstring>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "common/logger.h"
#include "common/status.h"
#include "generated/inode_generated.h"
#include "namenode/common/mutation.h"
#include "namenode/common/request_scoped_allocator.h"
#include "namenode/kv_store/column_family.h"
#include "namenode/kv_store/kv_store_base.h"
#include "namenode/path_resolver/path_resolver_base.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

bool operator==(const serde::File& lhs, const serde::FileT& rhs) {
  return lhs.length() == rhs.length;
}

bool operator==(const serde::Directory& lhs, const serde::DirectoryT& rhs) {
  return lhs.parent_inode_id() == rhs.parent_inode_id &&
         CHECK_NOTNULL(lhs.name())->str() == rhs.name;
}

bool operator==(const serde::Symlink& lhs, const serde::SymlinkT& rhs) {
  return CHECK_NOTNULL(lhs.target())->str() == rhs.target;
}

bool operator==(const serde::InodeBasicInfo& lhs,
                const serde::InodeBasicInfoT& rhs) {
  if (!(lhs.inode_id() == rhs.inode_id &&
        lhs.inode_type_type() == rhs.inode_type.type &&
        lhs.created_txid() == rhs.created_txid &&
        lhs.renamed_txid() == rhs.renamed_txid)) {
    return false;
  }
  switch (lhs.inode_type_type()) {
    case serde::InodeType_File:
      return *CHECK_NOTNULL(lhs.inode_type_as_File()) ==
             *CHECK_NOTNULL(rhs.inode_type.AsFile());
    case serde::InodeType_Directory:
      return *CHECK_NOTNULL(lhs.inode_type_as_Directory()) ==
             *CHECK_NOTNULL(rhs.inode_type.AsDirectory());
    case serde::InodeType_Symlink:
      return *CHECK_NOTNULL(lhs.inode_type_as_Symlink()) ==
             *CHECK_NOTNULL(rhs.inode_type.AsSymlink());
    default:
      return true;
  }
}

bool operator==(const serde::InodeTimestamps& lhs,
                const serde::InodeTimestampsT& rhs) {
  return lhs.ctime_in_nanoseconds() == rhs.ctime_in_nanoseconds &&
         lhs.mtime_in_nanoseconds() == rhs.mtime_in_nanoseconds &&
         lhs.atime_in_nanoseconds() == rhs.atime_in_nanoseconds;
}

InodeTable::InodeTable(TransactionBase* transaction,
                       RequestScopedAllocator allocator)
    : transaction_(transaction), allocator_(allocator) {
  CHECK_NOTNULL(transaction_);
}

std::expected<std::optional<Inode>, Status> InodeTable::Read(InodeID inode_id) {
  auto inode_basic_info_str =
      transaction_->Get(kInodeBasicInfoCFIndex, EncodeKey(inode_id));
  if (!inode_basic_info_str) {
    return std::nullopt;
  }
  CHECK(!inode_basic_info_str->empty());

  auto inode_timestamps_str =
      transaction_->Get(kInodeTimestampsCFIndex, EncodeKey(inode_id));
  CHECK(static_cast<bool>(inode_timestamps_str));
  CHECK(!inode_timestamps_str->empty());

  Inode inode(std::move(*inode_basic_info_str),
              std::move(*inode_timestamps_str));
  return inode;
}

void InodeTable::Write(const ResolvedPaths& resolved_paths) {
  for (const auto& inode_mutation : resolved_paths.GetInodeMutations()) {
    const auto& original = inode_mutation.GetOriginal();
    const auto& modified = inode_mutation.GetModified();
    if (std::holds_alternative<std::monostate>(modified)) {
      continue;
    }
    const auto& optional_modified = std::get<std::optional<InodeT>>(modified);
    if (optional_modified == std::nullopt) {
      if (original != std::nullopt) {
        auto original_key = EncodeKey(original->inode_id());
        transaction_->Delete(kInodeBasicInfoCFIndex, original_key);
        transaction_->Delete(kInodeTimestampsCFIndex, original_key);
      }
      continue;
    }
    const auto& not_null_modified = *optional_modified;
    auto modified_key =
        EncodeKey(not_null_modified.inode_id(), not_null_modified.name());
    if (original != std::nullopt) {
      auto original_key = EncodeKey(original->inode_id(), original->name());
      if (original_key != modified_key) {
        transaction_->Delete(kInodeBasicInfoCFIndex, original_key);
        transaction_->Delete(kInodeTimestampsCFIndex, original_key);
      }
    }
    if (!(original &&
          original->basic_info() == not_null_modified.basic_info())) {
      flatbuffers::FlatBufferBuilder fbb;
      fbb.Finish(InodeBasicInfo::Pack(fbb, &not_null_modified.basic_info()));
      auto value = std::string_view{
          reinterpret_cast<const char*>(fbb.GetBufferPointer()), fbb.GetSize()};
      transaction_->Put(kInodeBasicInfoCFIndex, modified_key, value);
    }
    if (!(original &&
          original->timestamps() == not_null_modified.timestamps())) {
      flatbuffers::FlatBufferBuilder fbb;
      fbb.Finish(InodeTimestamps::Pack(fbb, &not_null_modified.timestamps()));
      auto value = std::string_view{
          reinterpret_cast<const char*>(fbb.GetBufferPointer()), fbb.GetSize()};
      transaction_->Put(kInodeTimestampsCFIndex, modified_key, value);
    }
  }
}

std::pmr::string InodeTable::EncodeKey(InodeID inode_id) {
  std::pmr::string inode_id_str(allocator_);
  inode_id_str.resize(sizeof(inode_id));
  absl::big_endian::Store64(inode_id_str.data(), inode_id.index);
  return inode_id_str;
}

}  // namespace rocketfs
