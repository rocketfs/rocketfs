#pragma once

#include <cstdint>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <variant>

#include "common/status.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

class WriteGroupBase {
 public:
  WriteGroupBase() = default;
  WriteGroupBase(const WriteGroupBase&) = delete;
  WriteGroupBase(WriteGroupBase&&) = delete;
  WriteGroupBase& operator=(const WriteGroupBase&) = delete;
  WriteGroupBase& operator=(WriteGroupBase&&) = delete;
  virtual ~WriteGroupBase() = default;

  // For optimistic concurrency control:
  // - `std::nullptr_t`: Key must not exist.
  // - `std::nullopt`: Key must exist (value doesn't matter).
  // - `std::string_view value`: Key must exist with matching value.
  virtual void ConditionCheck(
      ColumnFamilyIndex cf_index,
      std::string_view key,
      std::variant<std::nullptr_t, std::optional<std::string_view>>
          expected) = 0;
  virtual void Put(ColumnFamilyIndex cf_index,
                   std::string_view key,
                   std::string_view value) = 0;
  virtual void Delete(ColumnFamilyIndex cf_index, std::string_view key) = 0;
};

class WriteBatchBase {
 public:
  WriteBatchBase() = default;
  WriteBatchBase(const WriteBatchBase&) = delete;
  WriteBatchBase(WriteBatchBase&&) = delete;
  WriteBatchBase& operator=(const WriteBatchBase&) = delete;
  WriteBatchBase& operator=(WriteBatchBase&&) = delete;
  virtual ~WriteBatchBase() = default;

  virtual WriteGroupBase* AddWriteGroup() = 0;
};

class SnapshotBase {
 public:
  SnapshotBase() = default;
  SnapshotBase(const SnapshotBase&) = delete;
  SnapshotBase(SnapshotBase&&) = delete;
  SnapshotBase& operator=(const SnapshotBase&) = delete;
  SnapshotBase& operator=(SnapshotBase&&) = delete;
  virtual ~SnapshotBase() = default;
};

class KVStoreBase {
 public:
  KVStoreBase() = default;
  KVStoreBase(const KVStoreBase&) = delete;
  KVStoreBase(KVStoreBase&&) = delete;
  KVStoreBase& operator=(const KVStoreBase&) = delete;
  KVStoreBase& operator=(KVStoreBase&&) = delete;
  virtual ~KVStoreBase() = default;

  virtual std::unique_ptr<SnapshotBase> GetSnapshot() = 0;
  virtual Status Read(SnapshotBase* snapshot,
                      ColumnFamilyIndex cf_index,
                      std::string_view key,
                      std::pmr::string* value) = 0;
  virtual std::unique_ptr<WriteBatchBase> CreateWriteBatch(
      std::pmr::memory_resource* memory_resource) = 0;
  virtual Status Write(std::unique_ptr<WriteBatchBase> write_batch) = 0;
};

}  // namespace rocketfs
