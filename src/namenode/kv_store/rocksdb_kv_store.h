#include <rocksdb/db.h>

#include <vector>

#include "namenode/kv_store/kv_store_base.h"

namespace rocketfs {

class RocksDBWriteBatch : public WriteBatchBase {
 public:
  explicit RocksDBWriteBatch(
      const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles);
  RocksDBWriteBatch(const RocksDBWriteBatch&) = delete;
  RocksDBWriteBatch(RocksDBWriteBatch&&) = delete;
  RocksDBWriteBatch& operator=(const RocksDBWriteBatch&) = delete;
  RocksDBWriteBatch& operator=(RocksDBWriteBatch&&) = delete;
  ~RocksDBWriteBatch() override = default;

  void Put(ColumnFamilyIndex cf_index,
           std::string_view key,
           std::string_view value) override;
  void Delete(ColumnFamilyIndex cf_index, std::string_view key) override;

  rocksdb::WriteBatch* GetWriteBatch();

 private:
  const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles_;
  rocksdb::WriteBatch write_batch_;
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
              std::string* value) override;
  Status Write(WriteBatchBase* write_batch) override;

 private:
  std::unique_ptr<rocksdb::DB> db_;
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
};

}  // namespace rocketfs
