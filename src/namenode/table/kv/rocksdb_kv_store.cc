// Copyright 2025 RocketFS

#include "namenode/table/kv/rocksdb_kv_store.h"

#include <gflags/gflags.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>
#include <rocksdb/iterator.h>
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
#include "namenode/table/kv/column_family.h"

namespace rocketfs {

DECLARE_string(rocksdb_kv_store_db_path);

RocksDBTxn::RocksDBTxn(
    rocksdb::DB* db,
    const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
    int64_t start_version,
    ReqScopedAlloc alloc)
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
      alloc_(alloc) {
}

unifex::task<std::expected<std::optional<std::pmr::string>, Status>>
RocksDBTxn::Get(CFIndex cf_index,
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
  if (status.ok()) {
    std::pmr::string value(alloc_);
    value.assign(pinnable_slice.data(), pinnable_slice.size());
    if (!exclude_from_read_conflict) {
      AddReadConflictKey(cf_index, key, value);
    }
    co_return value;
  }
  if (status.IsNotFound()) {
    if (!exclude_from_read_conflict) {
      AddReadConflictKey(cf_index, key, std::monostate{});
    }
    co_return std::nullopt;
  }
  co_return std::unexpected(Status::SystemError(status.ToString()));
}

void RocksDBTxn::AddReadConflictKey(
    CFIndex cf_index,
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

unifex::task<std::expected<std::pmr::vector<std::pmr::string>, Status>>
RocksDBTxn::GetRange(CFIndex cf_index,
                     std::string_view start_key,
                     std::string_view end_key,
                     size_t limit,
                     bool exclude_from_read_conflict) {
  rocksdb::ReadOptions read_options;
  read_options.snapshot = snapshot_.get();
  CHECK_NE(cf_index, kInvalidCFIndex);
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  std::pmr::vector<std::pmr::string> values(alloc_);
  auto iter = std::unique_ptr<rocksdb::Iterator>(
      db_->NewIterator(read_options, cf_handles_[cf_index.index]));
  for (iter->Seek(start_key);
       iter->Valid() && iter->key().ToStringView() < end_key &&
       values.size() < limit;
       iter->Next()) {
    values.emplace_back(iter->value().data(), iter->value().size());
  }
  if (!iter->status().ok()) {
    co_return std::unexpected(Status::SystemError(iter->status().ToString()));
  }
  if (!exclude_from_read_conflict) {
    AddReadConflictKeyRange(cf_index, start_key, end_key);
  }
  co_return values;
}

void RocksDBTxn::AddReadConflictKeyRange(CFIndex cf_index,
                                         std::string_view start_key,
                                         std::string_view end_key) {
}

void RocksDBTxn::Put(CFIndex cf_index,
                     std::string_view key,
                     std::string_view value) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  write_set_[std::make_pair(cf_index, std::string(key))] = value;
}

void RocksDBTxn::Del(CFIndex cf_index, std::string_view key) {
  CHECK_GE(cf_index.index, 0);
  CHECK_LT(cf_index.index, cf_handles_.size());
  write_set_[std::make_pair(cf_index, std::string(key))] = std::nullopt;
}

RocksDBConflictDetector::RocksDBConflictDetector(int64_t latest_purged_version)
    : latest_purged_version_(latest_purged_version) {
}

unifex::task<bool> RocksDBConflictDetector::IsConflictFree(
    std::shared_ptr<const RocksDBTxn> txn) {
  co_await mutex_.async_lock();

  CHECK_GT(txn->start_version_, 0);
  CHECK_GT(txn->commit_version_, txn->start_version_);
  if (txn->start_version_ < latest_purged_version_) {
    mutex_.unlock();
    co_return false;
  }

  // Ensuring no cycles in the direct serialization graph guarantees txn
  // serializability. [Weak Consistency: A Generalized Theory and Optimistic
  // Implementations for Distributed
  // Transactions](https://pmg.csail.mit.edu/papers/adya-phd.pdf) explains this
  // principle.
  // [FoundationDB](https://apple.github.io/foundationdb/developer-guide.html#how-foundationdb-detects-conflicts)
  // simplifies conflict detection with three steps:
  // 1. Assign a read version at the first read.
  // 2. Assign a commit version at commit.
  // 3. A txn is conflict free if and only if there have been no writes to any
  //    key that was read by that txn between the time the txn started and the
  //    commit time.
  bool is_conflict_free =
      !std::any_of(committed_txns_.lower_bound(txn->start_version_),
                   committed_txns_.upper_bound(txn->commit_version_),
                   [this, txn = txn.get()](const auto& p) {
                     const auto& [version, prev_committed_txn] = p;
                     CHECK_EQ(version, prev_committed_txn->commit_version_);
                     return HasConflict(*txn, *prev_committed_txn);
                   });
  LOG_DEBUG(logger,
            "Txn {} is conflict-free: {}.",
            txn->commit_version_,
            is_conflict_free);
  mutex_.unlock();
  co_return is_conflict_free;
}

bool RocksDBConflictDetector::HasConflict(
    const RocksDBTxn& txn, const RocksDBTxn& concurrent_txn) const {
  CHECK_LT(txn.start_version_, concurrent_txn.commit_version_);
  CHECK_GT(txn.commit_version_, concurrent_txn.commit_version_);
  return std::any_of(concurrent_txn.write_set_.begin(),
                     concurrent_txn.write_set_.end(),
                     [&txn](const auto& p) {
                       const auto& [key_with_cf, value] = p;
                       return txn.read_set_.find(key_with_cf) !=
                              txn.read_set_.end();
                     });
}

unifex::task<void> RocksDBConflictDetector::PurgeTo(int64_t version) {
  co_await mutex_.async_lock();
  committed_txns_.erase(committed_txns_.begin(),
                        committed_txns_.upper_bound(version));
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
      rocksdb::ColumnFamilyDescriptor(std::string(kInodeCFName), {}),
      rocksdb::ColumnFamilyDescriptor(std::string(kMTimeCFName), {}),
      rocksdb::ColumnFamilyDescriptor(std::string(kATimeCFName), {}),
      rocksdb::ColumnFamilyDescriptor(std::string(kDEntCFName), {})};
  auto status = rocksdb::DB::Open(
      options, "/tmp/rocksdb", cf_descriptors, &cf_handles_, &db);
  LOG_INFO(logger, "RocksDB open status: {}.", status.ToString());
  CHECK(status.ok());
  CHECK_EQ(cf_handles_.size(), cf_descriptors.size());
  for (auto [cf_index, cf_name] :
       std::initializer_list<std::pair<CFIndex, std::string_view>>{
           {kDefaultCFIndex, kDefaultCFName},
           {kInodeCFIndex, kInodeCFName},
           {kMTimeCFIndex, kMTimeCFName},
           {kATimeCFIndex, kATimeCFName},
           {kDEntCFIndex, kDEntCFName}}) {
    CHECK_EQ(cf_handles_[cf_index.index]->GetName(), cf_name);
  }
  db_ = std::unique_ptr<rocksdb::DB>(db);
}

std::unique_ptr<TxnBase> RocksDBKVStore::StartTxn(ReqScopedAlloc alloc) {
  return std::make_unique<RocksDBTxn>(
      db_.get(), cf_handles_, version_.fetch_add(1), alloc);
}

unifex::task<std::expected<void, Status>> RocksDBKVStore::CommitTxn(
    std::unique_ptr<TxnBase> txn) {
  auto rocksdb_txn =
      std::shared_ptr<RocksDBTxn>(dynamic_cast<RocksDBTxn*>(txn.release()));
  CHECK(static_cast<bool>(rocksdb_txn));
  rocksdb_txn->commit_version_ = version_.fetch_add(1);
  if (!co_await conflict_detector_.IsConflictFree(rocksdb_txn)) {
    LOG_DEBUG(logger,
              "Txn {} was aborted due to a conflict.",
              rocksdb_txn->commit_version_);
    co_return std::unexpected(Status::ConflictError());
  }
  rocksdb::WriteBatch write_batch;
  for (const auto& [key_with_cf, value] : rocksdb_txn->write_set_) {
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
