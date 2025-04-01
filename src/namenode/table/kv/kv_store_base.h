#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <variant>

#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/kv/column_family.h"

namespace rocketfs {

class TxnBase {
 public:
  TxnBase() = default;
  TxnBase(const TxnBase&) = delete;
  TxnBase(TxnBase&&) = delete;
  TxnBase& operator=(const TxnBase&) = delete;
  TxnBase& operator=(TxnBase&&) = delete;
  virtual ~TxnBase() = default;

  virtual unifex::task<std::expected<std::optional<std::pmr::string>, Status>>
  Get(CFIndex cf_index,
      std::string_view key,
      bool exclude_from_read_conflict = false) = 0;
  virtual void AddReadConflictKey(
      CFIndex cf_index,
      std::string_view key,
      // - `nullptr`: Key must not exist.
      // - `std::nullopt`: Key must exist (value doesn't matter).
      // - `std::string_view value`: Key must exist with matching value.
      std::variant<std::monostate, std::optional<std::string_view>> value) = 0;

  virtual unifex::task<
      std::expected<std::pmr::vector<std::pmr::string>, Status>>
  GetRange(CFIndex cf_index,
           std::string_view start_key,
           std::string_view end_key,
           size_t limit = std::numeric_limits<size_t>::max(),
           bool exclude_from_read_conflict = false) = 0;
  virtual void AddReadConflictKeyRange(CFIndex cf_index,
                                       // [start_key, end_key)
                                       std::string_view start_key,
                                       std::string_view end_key) = 0;

  virtual void Put(CFIndex cf_index,
                   std::string_view key,
                   std::string_view value) = 0;
  virtual void Del(CFIndex cf_index, std::string_view key) = 0;
};

class KVStoreBase {
 public:
  KVStoreBase() = default;
  KVStoreBase(const KVStoreBase&) = delete;
  KVStoreBase(KVStoreBase&&) = delete;
  KVStoreBase& operator=(const KVStoreBase&) = delete;
  KVStoreBase& operator=(KVStoreBase&&) = delete;
  virtual ~KVStoreBase() = default;

  virtual std::unique_ptr<TxnBase> StartTxn(ReqScopedAlloc alloc) = 0;
  virtual unifex::task<std::expected<void, Status>> CommitTxn(
      std::unique_ptr<TxnBase> txn) = 0;
};

}  // namespace rocketfs
