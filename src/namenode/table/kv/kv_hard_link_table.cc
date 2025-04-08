// Copyright 2025 RocketFS

#include "namenode/table/kv/kv_hard_link_table.h"

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

namespace rocketfs {

KVHardLinkTable::KVHardLinkTable(TxnBase* txn, ReqScopedAlloc alloc)
    : txn_(CHECK_NOTNULL(txn)), alloc_(alloc), dent_serde_(alloc_) {
}

unifex::task<std::expected<std::optional<HardLink>, Status>>
KVHardLinkTable::Read(InodeID parent_id, std::string_view name) {
  auto dent_str =
      co_await txn_->Get(kDEntCFIndex, dent_serde_.SerKey(parent_id, name));
  if (!dent_str) {
    co_return std::unexpected(Status::SystemError(fmt::format(
        "Failed to get dent for parent {} and name {}.", parent_id.val, name)));
  }
  if (!*dent_str) {
    co_return std::nullopt;
  }
  auto dent = dent_serde_.DeVal(**dent_str);
  if (!std::holds_alternative<HardLink>(dent)) {
    co_return std::nullopt;
  }
  co_return std::get<HardLink>(std::move(dent));
}

void KVHardLinkTable::Write(const std::optional<HardLink>& orig,
                            const std::optional<HardLink>& mod) {
  dent_serde_.Write(txn_, kDEntCFIndex, orig, mod);
}

}  // namespace rocketfs
