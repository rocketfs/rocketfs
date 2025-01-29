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

 private:
  const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles_;
  rocksdb::WriteBatch write_batch_;
};

class RocksDBKVStore : public KVStoreBase {
 public:
  RocksDBKVStore();
  RocksDBKVStore(const RocksDBKVStore&) = delete;
  RocksDBKVStore(RocksDBKVStore&&) = delete;
  RocksDBKVStore& operator=(const RocksDBKVStore&) = delete;
  RocksDBKVStore& operator=(RocksDBKVStore&&) = delete;
  ~RocksDBKVStore() override = default;

  Status Read(SnapshotBase* snapshot,
              ColumnFamilyIndex cf_index,
              std::string_view key,
              std::string* value) override;
  Status Write(WriteBatchBase* write_batch) override;
};

}  // namespace rocketfs
