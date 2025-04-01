// Copyright 2025 RocketFS

#pragma once

#include <expected>
#include <optional>
#include <string_view>

#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/kv/kv_store_base.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

class KVDirTable : public DirTableBase {
 public:
  KVDirTable(TxnBase* txn, ReqScopedAlloc alloc);
  KVDirTable(const KVDirTable&) = delete;
  KVDirTable(KVDirTable&&) = delete;
  KVDirTable& operator=(const KVDirTable&) = delete;
  KVDirTable& operator=(KVDirTable&&) = delete;
  ~KVDirTable() override = default;

  unifex::task<std::expected<std::optional<Dir>, Status>> Read(
      InodeID id) override;
  unifex::task<std::expected<std::optional<Dir>, Status>> Read(
      InodeID parent_id, std::string_view name) override;
  void Write(const std::optional<Dir>& original,
             const std::optional<Dir>& modified) override;

 private:
  TxnBase* txn_;
  ReqScopedAlloc alloc_;
  InodeSerde inode_serde_;
  MTimeSerde mtime_serde_;
  ATimeSerde atime_serde_;
  DEntSerde dent_serde_;
};

}  // namespace rocketfs
