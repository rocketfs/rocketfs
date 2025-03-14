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
#include "namenode/common/request_scoped_allocator.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

class TransactionBase {
 public:
  TransactionBase() = default;
  TransactionBase(const TransactionBase&) = delete;
  TransactionBase(TransactionBase&&) = delete;
  TransactionBase& operator=(const TransactionBase&) = delete;
  TransactionBase& operator=(TransactionBase&&) = delete;
  virtual ~TransactionBase() = default;

  virtual std::expected<std::pmr::string, Status> Get(
      ColumnFamilyIndex cf_index,
      std::string_view key,
      bool exclude_from_read_conflict = false) = 0;
  virtual void AddReadConflictKey(
      ColumnFamilyIndex cf_index,
      std::string_view key,
      // - `nullptr`: Key must not exist.
      // - `std::nullopt`: Key must exist (value doesn't matter).
      // - `std::string_view value`: Key must exist with matching value.
      std::variant<std::monostate, std::optional<std::string_view>> value) = 0;
  virtual void Put(ColumnFamilyIndex cf_index,
                   std::string_view key,
                   std::string_view value) = 0;
  virtual void Delete(ColumnFamilyIndex cf_index, std::string_view key) = 0;
};

class KVStoreBase {
 public:
  KVStoreBase() = default;
  KVStoreBase(const KVStoreBase&) = delete;
  KVStoreBase(KVStoreBase&&) = delete;
  KVStoreBase& operator=(const KVStoreBase&) = delete;
  KVStoreBase& operator=(KVStoreBase&&) = delete;
  virtual ~KVStoreBase() = default;

  virtual std::unique_ptr<TransactionBase> StartTransaction(
      RequestScopedAllocator allocator) = 0;
  virtual unifex::task<std::expected<void, Status>> CommitTransaction(
      std::unique_ptr<TransactionBase> transaction) = 0;
};

}  // namespace rocketfs
