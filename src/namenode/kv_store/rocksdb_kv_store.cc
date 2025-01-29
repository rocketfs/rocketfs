// Copyright 2025 RocketFS

#include "namenode/kv_store/rocksdb_kv_store.h"

#include <gflags/gflags.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>

#include <initializer_list>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>
#include <vector>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

DECLARE_string(rocksdb_kv_store_db_path);

RocksDBWriteGroup::RocksDBWriteGroup(
    const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
    rocksdb::WriteBatch* write_batch,
    std::pmr::vector<std::tuple<
        rocksdb::ColumnFamilyHandle*,
        std::string,
        std::variant<std::nullptr_t, std::optional<std::string_view>>>>*
        condition_check_items)
    : cf_handles_(cf_handles),
      write_batch_(write_batch),
      condition_check_items_(condition_check_items) {
  CHECK_NOTNULL(write_batch);
  CHECK_NOTNULL(condition_check_items);
}

void RocksDBWriteGroup::ConditionCheck(
    ColumnFamilyIndex cf_index,
    std::string_view key,
    std::variant<std::nullptr_t, std::optional<std::string_view>> expected) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  condition_check_items_->emplace_back(
      cf_handles_[cf_index.index], key, std::move(expected));
}

void RocksDBWriteGroup::Put(ColumnFamilyIndex cf_index,
                            std::string_view key,
                            std::string_view value) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  CHECK(write_batch_->Put(cf_handles_[cf_index.index], key, value).ok());
}

void RocksDBWriteGroup::Delete(ColumnFamilyIndex cf_index,
                               std::string_view key) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  CHECK(write_batch_->Delete(cf_handles_[cf_index.index], key).ok());
}

RocksDBWriteBatch::RocksDBWriteBatch(
    const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
    std::pmr::memory_resource* memory_resource)
    : cf_handles_(cf_handles),
      write_groups_(memory_resource),
      memory_resource_(memory_resource) {
  CHECK_NOTNULL(memory_resource_);
}

WriteGroupBase* RocksDBWriteBatch::AddWriteGroup() {
  auto write_group = std::make_unique<RocksDBWriteGroup>(
      cf_handles_, &write_batch_, &condition_check_items_);
  write_groups_.push_back(std::move(write_group));
  return write_groups_.back().get();
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
  std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors{
      rocksdb::ColumnFamilyDescriptor(std::string(kINodeBasicInfoCFName), {}),
      rocksdb::ColumnFamilyDescriptor(std::string(kINodeTimestampsCFName), {})};
  CHECK(rocksdb::DB::Open(
            options, "/tmp/rocksdb", cf_descriptors, &cf_handles_, &db)
            .ok());
  CHECK_EQ(cf_handles_.size(), cf_descriptors.size());
  for (auto [cf_index, cf_name] :
       std::initializer_list<std::pair<ColumnFamilyIndex, std::string_view>>{
           {kINodeBasicInfoCFIndex, kINodeBasicInfoCFName},
           {kINodeTimestampsCFIndex, kINodeTimestampsCFName}}) {
    CHECK_EQ(cf_handles_[cf_index.index]->GetName(), cf_name);
  }
  db_ = std::unique_ptr<rocksdb::DB>(db);
}

std::unique_ptr<SnapshotBase> RocksDBKVStore::GetSnapshot() {
  return std::make_unique<RocksDBSnapshot>(db_.get());
}

Status RocksDBKVStore::Read(SnapshotBase* snapshot,
                            ColumnFamilyIndex cf_index,
                            std::string_view key,
                            std::pmr::string* value) {
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
  rocksdb::PinnableSlice pinnable_slice;
  rocksdb::Status status =
      db_->Get(read_options, cf_handles_[cf_index.index], key, &pinnable_slice);
  CHECK(status.ok() || status.IsNotFound());
  if (!status.ok()) {
    return Status::NotFoundError();
  }
  value->assign(pinnable_slice.data(), pinnable_slice.size());
  return Status::OK();
}

std::unique_ptr<WriteBatchBase> RocksDBKVStore::CreateWriteBatch(
    std::pmr::memory_resource* memory_resource) {
  return std::make_unique<RocksDBWriteBatch>(cf_handles_, memory_resource);
}

Status RocksDBKVStore::Write(std::unique_ptr<WriteBatchBase> write_batch) {
  CHECK_NOTNULL(write_batch);
  auto rocksdb_write_batch =
      dynamic_cast<RocksDBWriteBatch*>(write_batch.get());
  CHECK_NOTNULL(rocksdb_write_batch);
  rocksdb::WriteOptions write_options;
  rocksdb::Status status =
      db_->Write(write_options, rocksdb_write_batch->GetWriteBatch());
  CHECK(status.ok());
  return Status::OK();
}

}  // namespace rocketfs
