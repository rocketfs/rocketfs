#include <rocksdb/db.h>
#include <rocksdb/snapshot.h>
#include <rocksdb/write_batch.h>

#include <functional>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>
#include <version>

#include "namenode/kv_store/kv_store_base.h"

namespace rocketfs {

class RocksDBWriteGroup : public WriteGroupBase {
 public:
  RocksDBWriteGroup(
      const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
      rocksdb::WriteBatch* write_batch,
      std::pmr::vector<std::tuple<
          rocksdb::ColumnFamilyHandle*,
          std::string,
          std::variant<std::nullptr_t, std::optional<std::string_view>>>>*
          condition_check_items);
  RocksDBWriteGroup(const RocksDBWriteGroup&) = delete;
  RocksDBWriteGroup(RocksDBWriteGroup&&) = delete;
  RocksDBWriteGroup& operator=(const RocksDBWriteGroup&) = delete;
  RocksDBWriteGroup& operator=(RocksDBWriteGroup&&) = delete;
  ~RocksDBWriteGroup() override = default;

  void ConditionCheck(
      ColumnFamilyIndex cf_index,
      std::string_view key,
      std::variant<std::nullptr_t, std::optional<std::string_view>> expected)
      override;
  void Put(ColumnFamilyIndex cf_index,
           std::string_view key,
           std::string_view value) override;
  void Delete(ColumnFamilyIndex cf_index, std::string_view key) override;

 private:
  const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles_;
  rocksdb::WriteBatch* write_batch_;
  std::pmr::vector<std::tuple<
      rocksdb::ColumnFamilyHandle*,
      std::string,
      std::variant<std::nullptr_t, std::optional<std::string_view>>>>*
      condition_check_items_;
};

class RocksDBWriteBatch : public WriteBatchBase {
 public:
  RocksDBWriteBatch(const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
                    std::pmr::memory_resource* memory_resource);
  RocksDBWriteBatch(const RocksDBWriteBatch&) = delete;
  RocksDBWriteBatch(RocksDBWriteBatch&&) = delete;
  RocksDBWriteBatch& operator=(const RocksDBWriteBatch&) = delete;
  RocksDBWriteBatch& operator=(RocksDBWriteBatch&&) = delete;
  ~RocksDBWriteBatch() override = default;

  WriteGroupBase* AddWriteGroup() override;
  rocksdb::WriteBatch* GetWriteBatch();

 private:
  const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles_;
  std::pmr::vector<std::unique_ptr<RocksDBWriteGroup>> write_groups_;
  std::pmr::memory_resource* memory_resource_;
  rocksdb::WriteBatch write_batch_;
  std::pmr::vector<
      std::tuple<rocksdb::ColumnFamilyHandle*,
                 std::string,
                 std::variant<std::nullptr_t, std::optional<std::string_view>>>>
      condition_check_items_;
};

class RocksDBSnapshot : public SnapshotBase {
 public:
  explicit RocksDBSnapshot(rocksdb::DB* db);
  RocksDBSnapshot(const RocksDBSnapshot&) = delete;
  RocksDBSnapshot(RocksDBSnapshot&&) = delete;
  RocksDBSnapshot& operator=(const RocksDBSnapshot&) = delete;
  RocksDBSnapshot& operator=(RocksDBSnapshot&&) = delete;
  ~RocksDBSnapshot() = default;

  const rocksdb::Snapshot* GetSnapshot();

 private:
  std::unique_ptr<const rocksdb::Snapshot,
                  std::function<void(const rocksdb::Snapshot*)>>
      snapshot_;
};

class RocksDBKVStore : public KVStoreBase {
 public:
  RocksDBKVStore();
  RocksDBKVStore(const RocksDBKVStore&) = delete;
  RocksDBKVStore(RocksDBKVStore&&) = delete;
  RocksDBKVStore& operator=(const RocksDBKVStore&) = delete;
  RocksDBKVStore& operator=(RocksDBKVStore&&) = delete;
  ~RocksDBKVStore() override = default;

  std::unique_ptr<SnapshotBase> GetSnapshot() override;
  Status Read(SnapshotBase* snapshot,
              ColumnFamilyIndex cf_index,
              std::string_view key,
              std::pmr::string* value) override;
  std::unique_ptr<WriteBatchBase> CreateWriteBatch(
      std::pmr::memory_resource* memory_resource) override;
  Status Write(std::unique_ptr<WriteBatchBase> write_batch) override;

 private:
  std::unique_ptr<rocksdb::DB> db_;
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
};

}  // namespace rocketfs
