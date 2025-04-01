#include <rocksdb/db.h>
#include <rocksdb/snapshot.h>

#include <atomic>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <unifex/async_mutex.hpp>
#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/kv/column_family.h"
#include "namenode/table/kv/kv_store_base.h"

namespace rocketfs {

constexpr std::string_view kDefaultCFName{"default"};
constexpr std::string_view kInodeCFName{"Inode"};
constexpr std::string_view kMTimeCFName{"DirectoryMTime"};
constexpr std::string_view kATimeCFName{"ATime"};
constexpr std::string_view kDEntCFName{"DEnt"};

class RocksDBTxn : public TxnBase {
  friend class RocksDBKVStore;
  friend class RocksDBConflictDetector;

  struct Comparator {
    bool operator()(const std::pair<CFIndex, std::string>& lhs,
                    const std::pair<CFIndex, std::string>& rhs) const {
      return lhs.first.index < rhs.first.index ||
             (lhs.first.index == rhs.first.index && lhs.second < rhs.second);
    }
  };

 public:
  RocksDBTxn(rocksdb::DB* db,
             const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
             int64_t start_version,
             ReqScopedAlloc alloc);
  RocksDBTxn(const RocksDBTxn&) = delete;
  RocksDBTxn(RocksDBTxn&&) = delete;
  RocksDBTxn& operator=(const RocksDBTxn&) = delete;
  RocksDBTxn& operator=(RocksDBTxn&&) = delete;
  ~RocksDBTxn() = default;

  unifex::task<std::expected<std::optional<std::pmr::string>, Status>> Get(
      CFIndex cf_index,
      std::string_view key,
      bool exclude_from_read_conflict) override;
  void AddReadConflictKey(
      CFIndex cf_index,
      std::string_view key,
      std::variant<std::monostate, std::optional<std::string_view>> value)
      override;

  unifex::task<std::expected<std::pmr::vector<std::pmr::string>, Status>>
  GetRange(CFIndex cf_index,
           std::string_view start_key,
           std::string_view end_key,
           size_t limit,
           bool exclude_from_read_conflict) override;
  void AddReadConflictKeyRange(CFIndex cf_index,
                               std::string_view start_key,
                               std::string_view end_key) override;

  void Put(CFIndex cf_index,
           std::string_view key,
           std::string_view value) override;
  void Del(CFIndex cf_index, std::string_view key) override;

 private:
  rocksdb::DB* db_;
  const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles_;
  std::unique_ptr<const rocksdb::Snapshot,
                  std::function<void(const rocksdb::Snapshot*)>>
      snapshot_;

  int64_t start_version_;
  int64_t commit_version_;
  std::map<std::pair<CFIndex, std::string>,
           std::variant<std::monostate, std::optional<std::string>>,
           Comparator>
      read_set_;
  std::map<std::pair<CFIndex, std::string>,
           std::optional<std::string>,
           Comparator>
      write_set_;

  ReqScopedAlloc alloc_;
};

class RocksDBConflictDetector {
 public:
  explicit RocksDBConflictDetector(int64_t latest_purged_version);
  RocksDBConflictDetector(RocksDBConflictDetector&&) = delete;
  RocksDBConflictDetector& operator=(const RocksDBConflictDetector&) = delete;
  RocksDBConflictDetector& operator=(RocksDBConflictDetector&&) = delete;
  ~RocksDBConflictDetector() = default;

  unifex::task<bool> IsConflictFree(std::shared_ptr<const RocksDBTxn> txn);

 private:
  bool HasConflict(const RocksDBTxn& txn,
                   const RocksDBTxn& concurrent_txn) const;
  unifex::task<void> PurgeTo(int64_t version);

 private:
  unifex::async_mutex mutex_;
  std::map<int64_t, std::shared_ptr<RocksDBTxn>> committed_txns_;
  int64_t latest_purged_version_;
};

class RocksDBKVStore : public KVStoreBase {
 public:
  RocksDBKVStore();
  RocksDBKVStore(const RocksDBKVStore&) = delete;
  RocksDBKVStore(RocksDBKVStore&&) = delete;
  RocksDBKVStore& operator=(const RocksDBKVStore&) = delete;
  RocksDBKVStore& operator=(RocksDBKVStore&&) = delete;
  ~RocksDBKVStore() override = default;

  std::unique_ptr<TxnBase> StartTxn(ReqScopedAlloc alloc) override;
  unifex::task<std::expected<void, Status>> CommitTxn(
      std::unique_ptr<TxnBase> txn) override;

 private:
  std::unique_ptr<rocksdb::DB> db_;
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
  std::atomic<int64_t> version_;
  RocksDBConflictDetector conflict_detector_;
};

}  // namespace rocketfs
