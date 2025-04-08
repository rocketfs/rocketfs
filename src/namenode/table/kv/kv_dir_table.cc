// Copyright 2025 RocketFS

#include "namenode/table/kv/kv_dir_table.h"

#include <fmt/base.h>
#include <sys/stat.h>

#include <coroutine>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/table/inode_id.h"
#include "namenode/table/kv/column_family.h"
#include "namenode/table/kv/kv_store_base.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

bool operator==(const std::optional<Dir>& lhs, const std::optional<Dir>& rhs) {
  if (static_cast<bool>(lhs) ^ static_cast<bool>(rhs)) {
    return false;
  }
  if (!lhs && !rhs) {
    return true;
  }
  return *lhs == *rhs;
}

KVDirTable::KVDirTable(TxnBase* txn, ReqScopedAlloc alloc)
    : txn_(CHECK_NOTNULL(txn)),
      alloc_(alloc),
      inode_serde_(alloc_),
      mtime_serde_(alloc_),
      atime_serde_(alloc_),
      dent_serde_(alloc_) {
}

unifex::task<std::expected<std::optional<Dir>, Status>> KVDirTable::Read(
    InodeID id) {
  auto inode_str = co_await txn_->Get(kInodeCFIndex, inode_serde_.SerKey(id));
  if (!inode_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get inode for inode {}.", id.val),
        inode_str.error()));
  }
  if (!*inode_str) {
    if (id == kRootInodeID) {
      co_return std::make_optional<Dir>(
          Dir{.parent_id = kRootInodeID,
              .name = "",
              .id = kRootInodeID,
              .acl = Acl{.uid = 0, .gid = 0, .perm = ALLPERMS},
              .ctime_in_ns = 0,
              .mtime_in_ns = 0,
              .atime_in_ns = 0});
    }
    co_return std::nullopt;
  }
  auto inode = inode_serde_.DeVal(**inode_str);
  if (!std::holds_alternative<Dir>(inode)) {
    co_return std::nullopt;
  }
  auto dir = std::get<Dir>(std::move(inode));

  auto mtime_str = co_await txn_->Get(kMTimeCFIndex, mtime_serde_.SerKey(id));
  if (!mtime_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get mtime for inode {}.", id.val),
        mtime_str.error()));
  }
  CHECK_NOTNULLOPT(*mtime_str);
  dir.mtime_in_ns = mtime_serde_.DeVal(**mtime_str);

  auto atime_str = co_await txn_->Get(kATimeCFIndex, atime_serde_.SerKey(id));
  if (!atime_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get atime for inode {}.", id.val),
        atime_str.error()));
  }
  CHECK_NOTNULLOPT(*atime_str);
  dir.atime_in_ns = atime_serde_.DeVal(**atime_str);
  co_return dir;
}

unifex::task<std::expected<std::optional<Dir>, Status>> KVDirTable::Read(
    InodeID parent_id, std::string_view name) {
  auto dent_str =
      co_await txn_->Get(kDEntCFIndex, dent_serde_.SerKey(parent_id, name));
  if (!dent_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get dir entry for parent inode {} and name {}.",
                    parent_id.val,
                    name),
        dent_str.error()));
  }
  if (!*dent_str) {
    co_return std::nullopt;
  }
  auto dent = dent_serde_.DeVal(**dent_str);
  if (!std::holds_alternative<Dir>(dent)) {
    co_return std::nullopt;
  }
  co_return std::get<Dir>(std::move(dent));
}

void KVDirTable::Write(const std::optional<Dir>& orig,
                       const std::optional<Dir>& mod) {
  inode_serde_.Write(txn_, kInodeCFIndex, orig, mod);
  mtime_serde_.Write(txn_, kMTimeCFIndex, orig, mod);
  atime_serde_.Write(txn_, kATimeCFIndex, orig, mod);
  dent_serde_.Write(txn_, kDEntCFIndex, orig, mod);
}

}  // namespace rocketfs
