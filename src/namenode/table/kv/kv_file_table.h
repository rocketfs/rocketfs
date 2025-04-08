// Copyright 2025 RocketFS

#pragma once

#include <expected>
#include <optional>

#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/file_table_base.h"
#include "namenode/table/kv/kv_store_base.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

class KVFileTable : public FileTableBase {
 public:
  KVFileTable(TxnBase* txn, ReqScopedAlloc alloc);
  KVFileTable(const KVFileTable&) = delete;
  KVFileTable(KVFileTable&&) = delete;
  KVFileTable& operator=(const KVFileTable&) = delete;
  KVFileTable& operator=(KVFileTable&&) = delete;
  ~KVFileTable() override = default;

  unifex::task<std::expected<std::optional<File>, Status>> Read(
      InodeID id) override;
  void Write(const std::optional<File>& orig,
             const std::optional<File>& mod) override;

 private:
  TxnBase* txn_;
  ReqScopedAlloc alloc_;
  InodeSerde inode_serde_;
  MTimeSerde mtime_serde_;
  ATimeSerde atime_serde_;
};

}  // namespace rocketfs
