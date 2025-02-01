#include "namenode/kv_store/rocksdb_kv_store.h"

#include "common/logger.h"

namespace rocketfs {

RocksDBWriteBatch::RocksDBWriteBatch(
    const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles)
    : cf_handles_(cf_handles) {
}

void RocksDBWriteBatch::Put(ColumnFamilyIndex cf_index,
                            std::string_view key,
                            std::string_view value) {
  CHECK_GE(cf_index.GetIndex(), 0);
  CHECK_LT(cf_index.GetIndex(), cf_handles_.size());
  CHECK(write_batch_.Put(cf_handles_[cf_index.GetIndex()], key, value).ok());
}

void RocksDBWriteBatch::Delete(ColumnFamilyIndex cf_index,
                               std::string_view key) {
  CHECK_GE(cf_index.GetIndex(), 0);
  CHECK_LT(cf_index.GetIndex(), cf_handles_.size());
  CHECK(write_batch_.Delete(cf_handles_[cf_index.GetIndex()], key).ok());
}

}  // namespace rocketfs
