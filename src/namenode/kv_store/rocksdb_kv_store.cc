#include "namenode/kv_store/rocksdb_kv_store.h"

#include <gflags/gflags.h>

#include "common/logger.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

DECLARE_string(rocksdb_kv_store_db_path);

RocksDBWriteBatch::RocksDBWriteBatch(
    const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles)
    : cf_handles_(cf_handles) {
}

void RocksDBWriteBatch::Put(ColumnFamilyIndex cf_index,
                            std::string_view key,
                            std::string_view value) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  CHECK(write_batch_.Put(cf_handles_[cf_index.index], key, value).ok());
}

void RocksDBWriteBatch::Delete(ColumnFamilyIndex cf_index,
                               std::string_view key) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  CHECK(write_batch_.Delete(cf_handles_[cf_index.index], key).ok());
}

rocksdb::WriteBatch* RocksDBWriteBatch::GetWriteBatch() {
  return &write_batch_;
}

RocksDBSnapshot::RocksDBSnapshot(rocksdb::DB* db) {
  CHECK_NOTNULL(db);
  auto snapshot = db->GetSnapshot();
  CHECK_NOTNULL(snapshot);
  snapshot_ = decltype(snapshot_)(snapshot, [db](const auto* snapshot) {
    CHECK_NOTNULL(db);
    CHECK_NOTNULL(snapshot);
    db->ReleaseSnapshot(snapshot);
  });
}

const rocksdb::Snapshot* RocksDBSnapshot::GetSnapshot() {
  CHECK_NOTNULL(snapshot_);
  return snapshot_.get();
}

RocksDBKVStore::RocksDBKVStore() {
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::DB* db = nullptr;
  std::vector<rocksdb::ColumnFamilyDescriptor> column_family_descriptors{};
  CHECK(
      rocksdb::DB::Open(
          options, "/tmp/rocksdb", column_family_descriptors, &cf_handles_, &db)
          .ok());
  db_ = std::unique_ptr<rocksdb::DB>(db);
}

std::unique_ptr<SnapshotBase> RocksDBKVStore::GetSnapshot() {
  return std::make_unique<RocksDBSnapshot>(db_.get());
}

Status RocksDBKVStore::Read(SnapshotBase* snapshot,
                            ColumnFamilyIndex cf_index,
                            std::string_view key,
                            std::string* value) {
  CHECK_NOTNULL(value);
  rocksdb::ReadOptions read_options;
  if (snapshot != nullptr) {
    auto rocksdb_snapshot = dynamic_cast<RocksDBSnapshot*>(snapshot);
    CHECK_NOTNULL(rocksdb_snapshot);
    read_options.snapshot = rocksdb_snapshot->GetSnapshot();
  }
  CHECK_NE(cf_index, kInvalidCFIndex);
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  rocksdb::Status status =
      db_->Get(read_options, cf_handles_[cf_index.index], key, value);
  CHECK(status.ok() || status.IsNotFound());
  if (!status.ok()) {
    value->clear();
  }
  return status.IsNotFound() ? Status::NotFoundError() : Status::OK();
}

Status RocksDBKVStore::Write(WriteBatchBase* write_batch) {
  CHECK_NOTNULL(write_batch);
  auto rocksdb_write_batch = dynamic_cast<RocksDBWriteBatch*>(write_batch);
  CHECK_NOTNULL(rocksdb_write_batch);
  rocksdb::WriteOptions write_options;
  rocksdb::Status status =
      db_->Write(write_options, rocksdb_write_batch->GetWriteBatch());
  CHECK(status.ok());
  return Status::OK();
}

}  // namespace rocketfs
