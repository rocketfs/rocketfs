// Copyright 2025 RocketFS

#pragma once

#include <expected>
#include <optional>
#include <string_view>

#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/kv/kv_store_base.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

class KVHardLinkTable : public HardLinkTableBase {
 public:
  KVHardLinkTable(TxnBase* txn, ReqScopedAlloc alloc);
  KVHardLinkTable(const KVHardLinkTable&) = delete;
  KVHardLinkTable(KVHardLinkTable&&) = delete;
  KVHardLinkTable& operator=(const KVHardLinkTable&) = delete;
  KVHardLinkTable& operator=(KVHardLinkTable&&) = delete;
  ~KVHardLinkTable() override = default;

  unifex::task<std::expected<std::optional<HardLink>, Status>> Read(
      InodeID parent_id, std::string_view name) override;
  void Write(const std::optional<HardLink>& orig,
             const std::optional<HardLink>& mod) override;

 private:
  TxnBase* txn_;
  ReqScopedAlloc alloc_;
  DEntSerde dent_serde_;
};

}  // namespace rocketfs
