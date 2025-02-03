#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "common/status.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

class WriteBatchBase {
 public:
  WriteBatchBase() = default;
  WriteBatchBase(const WriteBatchBase&) = delete;
  WriteBatchBase(WriteBatchBase&&) = delete;
  WriteBatchBase& operator=(const WriteBatchBase&) = delete;
  WriteBatchBase& operator=(WriteBatchBase&&) = delete;
  virtual ~WriteBatchBase() = default;

  virtual void Put(ColumnFamilyIndex cf_index,
                   std::string_view key,
                   std::string_view value) = 0;
  virtual void Delete(ColumnFamilyIndex cf_index, std::string_view key) = 0;
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
  virtual std::unique_ptr<WriteBatchBase> CreateWriteBatch() = 0;
  virtual Status Write(std::unique_ptr<WriteBatchBase> write_batch) = 0;
};

}  // namespace rocketfs
