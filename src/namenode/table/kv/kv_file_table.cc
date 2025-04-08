// Copyright 2025 RocketFS

#include "namenode/table/kv/kv_file_table.h"

#include <fmt/base.h>

#include <coroutine>
#include <memory_resource>
#include <string>
#include <utility>
#include <variant>

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "namenode/table/inode_id.h"
#include "namenode/table/kv/column_family.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

KVFileTable::KVFileTable(TxnBase* txn, ReqScopedAlloc alloc)
    : txn_(CHECK_NOTNULL(txn)),
      alloc_(alloc),
      inode_serde_(alloc_),
      mtime_serde_(alloc_),
      atime_serde_(alloc_) {
}

unifex::task<std::expected<std::optional<File>, Status>> KVFileTable::Read(
    InodeID id) {
  auto inode_str = co_await txn_->Get(kInodeCFIndex, inode_serde_.SerKey(id));
  if (!inode_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get inode for inode {}.", id.val),
        inode_str.error()));
  }
  if (!*inode_str) {
    co_return std::nullopt;
  }
  auto inode = inode_serde_.DeVal(**inode_str);
  if (!std::holds_alternative<File>(inode)) {
    co_return std::nullopt;
  }
  auto file = std::get<File>(std::move(inode));

  auto mtime_str = co_await txn_->Get(kMTimeCFIndex, mtime_serde_.SerKey(id));
  if (!mtime_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get mtime for inode {}.", id.val),
        mtime_str.error()));
  }
  CHECK_NOTNULLOPT(*mtime_str);
  file.mtime_in_ns = mtime_serde_.DeVal(**mtime_str);

  auto atime_str = co_await txn_->Get(kATimeCFIndex, atime_serde_.SerKey(id));
  if (!atime_str) {
    co_return std::unexpected(Status::SystemError(
        fmt::format("Failed to get atime for inode {}.", id.val),
        atime_str.error()));
  }
  CHECK_NOTNULLOPT(*atime_str);
  file.atime_in_ns = atime_serde_.DeVal(**atime_str);
  co_return file;
}

void KVFileTable::Write(const std::optional<File>& orig,
                        const std::optional<File>& mod) {
  inode_serde_.Write(txn_, kInodeCFIndex, orig, mod);
  mtime_serde_.Write(txn_, kMTimeCFIndex, orig, mod);
  atime_serde_.Write(txn_, kATimeCFIndex, orig, mod);
}

}  // namespace rocketfs
