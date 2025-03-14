// Copyright 2025 RocketFS

#include "namenode/kv_store/rocksdb_kv_store.h"

#include <gflags/gflags.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <rocksdb/write_batch.h>

#include <algorithm>
#include <coroutine>
#include <expected>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>
#include <vector>

#include <unifex/coroutine.hpp>
#include <unifex/detail/with_type_erased_tag_invoke.hpp>
#include <unifex/overload.hpp>
#include <unifex/sender_for.hpp>
#include <unifex/unstoppable.hpp>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/kv_store/column_family.h"

namespace rocketfs {

DECLARE_string(rocksdb_kv_store_db_path);

RocksDBTransaction::RocksDBTransaction(
    rocksdb::DB* db,
    const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
    int64_t start_version,
    RequestScopedAllocator allocator)
    : db_(db),
      cf_handles_(cf_handles),
      snapshot_(CHECK_NOTNULL(CHECK_NOTNULL(db_)->GetSnapshot()),
                [db](const auto* snapshot) {
                  CHECK_NOTNULL(db);
                  CHECK_NOTNULL(snapshot);
                  db->ReleaseSnapshot(snapshot);
                }),
      start_version_(start_version),
      commit_version_(-1),
      allocator_(allocator) {
}

std::expected<std::pmr::string, Status> RocksDBTransaction::Get(
    ColumnFamilyIndex cf_index,
    std::string_view key,
    bool exclude_from_read_conflict) {
  rocksdb::ReadOptions read_options;
  read_options.snapshot = snapshot_.get();
  CHECK_NE(cf_index, kInvalidCFIndex);
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  rocksdb::PinnableSlice pinnable_slice;
  rocksdb::Status status =
      db_->Get(read_options, cf_handles_[cf_index.index], key, &pinnable_slice);
  CHECK(status.ok() || status.IsNotFound());

  if (!status.ok()) {
    if (!exclude_from_read_conflict) {
      AddReadConflictKey(cf_index, key, std::monostate{});
    }
    return std::unexpected(Status::NotFoundError());
  }

  std::pmr::string value(allocator_);
  value.assign(pinnable_slice.data(), pinnable_slice.size());
  if (!exclude_from_read_conflict) {
    AddReadConflictKey(cf_index, key, value);
  }
  return value;
}

void RocksDBTransaction::AddReadConflictKey(
    ColumnFamilyIndex cf_index,
    std::string_view key,
    std::variant<std::monostate, std::optional<std::string_view>> value) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  read_set_[std::make_pair(cf_index, std::string(key))] =
      value.index() == 0
          ? std::nullopt
          : std::get<1>(value).transform(
                [this](std::string_view value) { return std::string(value); });
}

void RocksDBTransaction::Put(ColumnFamilyIndex cf_index,
                             std::string_view key,
                             std::string_view value) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  write_set_[std::make_pair(cf_index, std::string(key))] = value;
}

void RocksDBTransaction::Delete(ColumnFamilyIndex cf_index,
                                std::string_view key) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  write_set_[std::make_pair(cf_index, std::string(key))] = std::nullopt;
}

RocksDBConflictDetector::RocksDBConflictDetector(int64_t latest_purged_version)
    : latest_purged_version_(latest_purged_version) {
}

unifex::task<bool> RocksDBConflictDetector::IsConflictFree(
    std::shared_ptr<const RocksDBTransaction> transaction) {
  co_await mutex_.async_lock();

  CHECK_GT(transaction->start_version_, 0);
  CHECK_GT(transaction->commit_version_, transaction->start_version_);
  if (transaction->start_version_ < latest_purged_version_) {
    mutex_.unlock();
    co_return false;
  }

  // Ensuring no cycles in the direct serialization graph guarantees transaction
  // serializability. [Weak Consistency: A Generalized Theory and Optimistic
  // Implementations for Distributed
  // Transactions](https://pmg.csail.mit.edu/papers/adya-phd.pdf) explains this
  // principle.
  // [FoundationDB](https://apple.github.io/foundationdb/developer-guide.html#how-foundationdb-detects-conflicts)
  // simplifies conflict detection with three steps:
  // 1. Assign a read version at the first read.
  // 2. Assign a commit version at commit.
  // 3. A transaction is conflict free if and only if there have been no writes
  //    to any key that was read by that transaction between the time the
  //    transaction started and the commit time.
  bool is_conflict_free = !std::any_of(
      committed_transactions_.lower_bound(transaction->start_version_),
      committed_transactions_.upper_bound(transaction->commit_version_),
      [this, transaction = transaction.get()](const auto& p) {
        const auto& [version, previous_committed_transaction] = p;
        CHECK_EQ(version, previous_committed_transaction->commit_version_);
        return HasConflict(*transaction, *previous_committed_transaction);
      });
  LOG_DEBUG(logger,
            "Transaction {} is conflict-free: {}.",
            transaction->commit_version_,
            is_conflict_free);
  mutex_.unlock();
  co_return is_conflict_free;
}

bool RocksDBConflictDetector::HasConflict(
    const RocksDBTransaction& transaction,
    const RocksDBTransaction& concurrent_transaction) const {
  CHECK_LT(transaction.start_version_, concurrent_transaction.commit_version_);
  CHECK_GT(transaction.commit_version_, concurrent_transaction.commit_version_);
  return std::any_of(concurrent_transaction.write_set_.begin(),
                     concurrent_transaction.write_set_.end(),
                     [&transaction](const auto& p) {
                       const auto& [key_with_cf, value] = p;
                       return transaction.read_set_.find(key_with_cf) !=
                              transaction.read_set_.end();
                     });
}

unifex::task<void> RocksDBConflictDetector::PurgeTo(int64_t version) {
  co_await mutex_.async_lock();
  committed_transactions_.erase(committed_transactions_.begin(),
                                committed_transactions_.upper_bound(version));
  if (version > latest_purged_version_) {
    latest_purged_version_ = version;
  }
  mutex_.unlock();
}

RocksDBKVStore::RocksDBKVStore()
    : version_(1), conflict_detector_(version_ - 1) {
  rocksdb::Options options;
  options.create_if_missing = true;
  options.create_missing_column_families = true;
  rocksdb::DB* db = nullptr;
  std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors{
      rocksdb::ColumnFamilyDescriptor(std::string(kDefaultCFName), {}),
      rocksdb::ColumnFamilyDescriptor(std::string(kInodeBasicInfoCFName), {}),
      rocksdb::ColumnFamilyDescriptor(std::string(kInodeTimestampsCFName), {})};
  auto status = rocksdb::DB::Open(
      options, "/tmp/rocksdb", cf_descriptors, &cf_handles_, &db);
  LOG_INFO(logger, "RocksDB open status: {}.", status.ToString());
  CHECK(status.ok());
  CHECK_EQ(cf_handles_.size(), cf_descriptors.size());
  for (auto [cf_index, cf_name] :
       std::initializer_list<std::pair<ColumnFamilyIndex, std::string_view>>{
           {kInodeBasicInfoCFIndex, kInodeBasicInfoCFName},
           {kInodeTimestampsCFIndex, kInodeTimestampsCFName}}) {
    CHECK_EQ(cf_handles_[cf_index.index]->GetName(), cf_name);
  }
  db_ = std::unique_ptr<rocksdb::DB>(db);
}

std::unique_ptr<TransactionBase> RocksDBKVStore::StartTransaction(
    RequestScopedAllocator allocator) {
  return std::make_unique<RocksDBTransaction>(
      db_.get(), cf_handles_, version_.fetch_add(1), allocator);
}

unifex::task<std::expected<void, Status>> RocksDBKVStore::CommitTransaction(
    std::unique_ptr<TransactionBase> transaction) {
  auto rocksdb_transaction = std::shared_ptr<RocksDBTransaction>(
      dynamic_cast<RocksDBTransaction*>(transaction.release()));
  CHECK(static_cast<bool>(rocksdb_transaction));
  rocksdb_transaction->commit_version_ = version_.fetch_add(1);
  if (!co_await conflict_detector_.IsConflictFree(rocksdb_transaction)) {
    LOG_DEBUG(logger,
              "Transaction {} was aborted due to a conflict.",
              rocksdb_transaction->commit_version_);
    co_return std::unexpected(Status::ConflictError());
  }
  rocksdb::WriteBatch write_batch;
  for (const auto& [key_with_cf, value] : rocksdb_transaction->write_set_) {
    const auto& [cf_index, key] = key_with_cf;
    if (value) {
      write_batch.Put(cf_handles_[cf_index.index], key, *value);
    } else {
      write_batch.Delete(cf_handles_[cf_index.index], key);
    }
  }
  CHECK(db_->Write(rocksdb::WriteOptions(), &write_batch).ok());
  co_return std::expected<void, Status>();
}

}  // namespace rocketfs
