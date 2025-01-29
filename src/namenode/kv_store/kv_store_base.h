#include <cstdint>
#include <string_view>

#include "common/status.h"

namespace rocketfs {

class ColumnFamilyIndex {
 public:
  ColumnFamilyIndex();
  explicit ColumnFamilyIndex(int32_t index);
  ColumnFamilyIndex(const ColumnFamilyIndex&) = default;
  ColumnFamilyIndex(ColumnFamilyIndex&&) = default;
  ColumnFamilyIndex& operator=(const ColumnFamilyIndex&) = default;
  ColumnFamilyIndex& operator=(ColumnFamilyIndex&&) = default;
  ~ColumnFamilyIndex() = default;

  int32_t GetIndex() const;

 private:
  int32_t index_;
};

extern const ColumnFamilyIndex kInvalidCFIndex;

class WriteBatchBase {
 public:
  WriteBatchBase() = default;
  WriteBatchBase(const WriteBatchBase&) = delete;
  WriteBatchBase(WriteBatchBase&&) = delete;
  WriteBatchBase& operator=(const WriteBatchBase&) = delete;
  WriteBatchBase& operator=(WriteBatchBase&&) = delete;
  virtual ~WriteBatchBase() = default;

  virtual void Put(ColumnFamilyIndex cf_index,
                   std::string_view key,
                   std::string_view value) = 0;
  virtual void Delete(ColumnFamilyIndex cf_index, std::string_view key) = 0;
};

class SnapshotBase {
 public:
  SnapshotBase() = default;
  SnapshotBase(const SnapshotBase&) = delete;
  SnapshotBase(SnapshotBase&&) = delete;
  SnapshotBase& operator=(const SnapshotBase&) = delete;
  SnapshotBase& operator=(SnapshotBase&&) = delete;
  virtual ~SnapshotBase() = default;
};

class KVStoreBase {
 public:
  KVStoreBase() = default;
  KVStoreBase(const KVStoreBase&) = delete;
  KVStoreBase(KVStoreBase&&) = delete;
  KVStoreBase& operator=(const KVStoreBase&) = delete;
  KVStoreBase& operator=(KVStoreBase&&) = delete;
  virtual ~KVStoreBase() = default;

  virtual Status Read(SnapshotBase* snapshot,
                      ColumnFamilyIndex cf_index,
                      std::string_view key,
                      std::string* value) = 0;
  virtual Status Write(WriteBatchBase* write_batch) = 0;
};

}  // namespace rocketfs
