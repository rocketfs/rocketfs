// Copyright 2025 RocketFS

#pragma once

#include <cstddef>
#include <expected>
#include <memory_resource>
#include <string_view>
#include <variant>
#include <vector>

#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/dent_view_base.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/kv/kv_store_base.h"
#include "namenode/table/kv/serde.h"

namespace rocketfs {

class KVDEntView : public DEntViewBase {
 public:
  KVDEntView(TxnBase* txn, ReqScopedAlloc alloc);
  KVDEntView(const KVDEntView&) = delete;
  KVDEntView(KVDEntView&&) = delete;
  KVDEntView& operator=(const KVDEntView&) = delete;
  KVDEntView& operator=(KVDEntView&&) = delete;
  ~KVDEntView() override = default;

  unifex::task<
      std::expected<std::variant<std::monostate, Dir, HardLink>, Status>>
  Read(InodeID parent_id, std::string_view name) override;
  unifex::task<
      std::expected<std::pmr::vector<std::variant<Dir, HardLink>>, Status>>
  List(InodeID parent_id, std::string_view start_after, size_t limit) override;

 private:
  TxnBase* txn_;
  ReqScopedAlloc alloc_;
  DEntSerde dent_serde_;
};

}  // namespace rocketfs
