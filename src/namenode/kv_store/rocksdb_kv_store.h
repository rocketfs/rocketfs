#include <rocksdb/db.h>
#include <rocksdb/snapshot.h>

#include <atomic>
#include <compare>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <unifex/async_mutex.hpp>
#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/common/request_scoped_allocator.h"
#include "namenode/kv_store/column_family.h"
#include "namenode/kv_store/kv_store_base.h"

namespace rocketfs {

constexpr std::string_view kDefaultCFName{"default"};
constexpr std::string_view kInodeBasicInfoCFName{"InodeBasicInfo"};
constexpr std::string_view kInodeTimestampsCFName{"InodeTimestamps"};

class RocksDBTransaction : public TransactionBase {
  friend class RocksDBKVStore;
  friend class RocksDBConflictDetector;

  struct Comparator {
    bool operator()(
        const std::pair<ColumnFamilyIndex, std::string>& lhs,
        const std::pair<ColumnFamilyIndex, std::string>& rhs) const {
      return lhs.first.index < rhs.first.index ||
             (lhs.first.index == rhs.first.index && lhs.second < rhs.second);
    }
  };

 public:
  RocksDBTransaction(
      rocksdb::DB* db,
      const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles,
      int64_t start_version,
      RequestScopedAllocator allocator);
  RocksDBTransaction(const RocksDBTransaction&) = delete;
  RocksDBTransaction(RocksDBTransaction&&) = delete;
  RocksDBTransaction& operator=(const RocksDBTransaction&) = delete;
  RocksDBTransaction& operator=(RocksDBTransaction&&) = delete;
  ~RocksDBTransaction() = default;

  std::expected<std::pmr::string, Status> Get(
      ColumnFamilyIndex cf_index,
      std::string_view key,
      bool exclude_from_read_conflict) override;
  void AddReadConflictKey(
      ColumnFamilyIndex cf_index,
      std::string_view key,
      std::variant<std::monostate, std::optional<std::string_view>> value)
      override;
  void Put(ColumnFamilyIndex cf_index,
           std::string_view key,
           std::string_view value) override;
  void Delete(ColumnFamilyIndex cf_index, std::string_view key) override;

 private:
  rocksdb::DB* db_;
  const std::vector<rocksdb::ColumnFamilyHandle*>& cf_handles_;
  std::unique_ptr<const rocksdb::Snapshot,
                  std::function<void(const rocksdb::Snapshot*)>>
      snapshot_;

  int64_t start_version_;
  int64_t commit_version_;
  std::map<std::pair<ColumnFamilyIndex, std::string>,
           std::variant<std::monostate, std::optional<std::string>>,
           Comparator>
      read_set_;
  std::map<std::pair<ColumnFamilyIndex, std::string>,
           std::optional<std::string>,
           Comparator>
      write_set_;

  RequestScopedAllocator allocator_;
};

class RocksDBConflictDetector {
 public:
  explicit RocksDBConflictDetector(int64_t latest_purged_version);
  RocksDBConflictDetector(RocksDBConflictDetector&&) = delete;
  RocksDBConflictDetector& operator=(const RocksDBConflictDetector&) = delete;
  RocksDBConflictDetector& operator=(RocksDBConflictDetector&&) = delete;
  ~RocksDBConflictDetector() = default;

  unifex::task<bool> IsConflictFree(
      std::shared_ptr<const RocksDBTransaction> transaction);

 private:
  bool HasConflict(const RocksDBTransaction& transaction,
                   const RocksDBTransaction& concurrent_transaction) const;
  unifex::task<void> PurgeTo(int64_t version);

 private:
  unifex::async_mutex mutex_;
  std::map<int64_t, std::shared_ptr<RocksDBTransaction>>
      committed_transactions_;
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

  std::unique_ptr<TransactionBase> StartTransaction(
      RequestScopedAllocator allocator) override;
  unifex::task<std::expected<void, Status>> CommitTransaction(
      std::unique_ptr<TransactionBase> transaction) override;

 private:
  std::unique_ptr<rocksdb::DB> db_;
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
  std::atomic<int64_t> version_;
  RocksDBConflictDetector conflict_detector_;
};

}  // namespace rocketfs
