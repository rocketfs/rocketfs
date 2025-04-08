// Copyright 2025 RocketFS

#include "namenode/table/kv/kv_dent_view.h"

#include <fmt/base.h>

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
#include "namenode/table/dir_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/inode_id.h"
#include "namenode/table/kv/column_family.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

KVDEntView::KVDEntView(TxnBase* txn, ReqScopedAlloc alloc)
    : txn_(CHECK_NOTNULL(txn)), alloc_(alloc), dent_serde_(alloc_) {
}

unifex::task<std::expected<std::variant<std::monostate, Dir, HardLink>, Status>>
KVDEntView::Read(InodeID parent_id, std::string_view name) {
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
    co_return std::monostate{};
  }
  co_return std::visit(
      [](auto&& arg) -> std::variant<std::monostate, Dir, HardLink> {
        return std::forward<decltype(arg)>(arg);
      },
      dent_serde_.DeVal(**dent_str));
}

unifex::task<
    std::expected<std::pmr::vector<std::variant<Dir, HardLink>>, Status>>
KVDEntView::List(InodeID parent_id,
                 std::string_view start_after,
                 size_t limit) {
  auto dent_strs =
      co_await txn_->GetRange(kDEntCFIndex,
                              dent_serde_.SerKey(parent_id, start_after),
                              dent_serde_.SerKey(parent_id, "\xFF"),
                              limit);
  if (!dent_strs) {
    co_return std::unexpected(Status::SystemError(
        fmt::format(
            "Failed to get dir entries for parent inode {} and start after {}.",
            parent_id.val,
            start_after),
        dent_strs.error()));
  }
  std::pmr::vector<std::variant<Dir, HardLink>> dents(alloc_);
  dents.reserve(dent_strs->size());
  for (const auto& dent_str : *dent_strs) {
    dents.emplace_back(dent_serde_.DeVal(dent_str));
  }
  co_return dents;
}

}  // namespace rocketfs
