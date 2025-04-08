// Copyright 2025 RocketFS

#pragma once

#include <flatbuffers/allocator.h>
#include <flatbuffers/detached_buffer.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "namenode/common/req_scoped_alloc.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/file_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/inode_id.h"
#include "namenode/table/kv/column_family.h"
#include "namenode/table/kv/kv_store_base.h"

namespace rocketfs {

class FBAllocator : public flatbuffers::Allocator {
 public:
  explicit FBAllocator(ReqScopedAlloc alloc);
  FBAllocator(const FBAllocator&) = delete;
  FBAllocator(FBAllocator&&) = delete;
  FBAllocator& operator=(const FBAllocator&) = delete;
  FBAllocator& operator=(FBAllocator&&) = delete;
  ~FBAllocator() override = default;

  uint8_t* allocate(size_t size) override;
  void deallocate(uint8_t* p, size_t size) override;

 private:
  std::allocator_traits<ReqScopedAlloc>::template rebind_alloc<uint8_t> alloc_;
};

// `DetachedBuffer` objects must not outlive their parent `Serde` instance.
class DetachedBuffer {
 public:
  DetachedBuffer(flatbuffers::DetachedBuffer&& buffer);
  DetachedBuffer(const DetachedBuffer&) = delete;
  DetachedBuffer(DetachedBuffer&& other) = default;
  DetachedBuffer& operator=(const DetachedBuffer&) = delete;
  DetachedBuffer& operator=(DetachedBuffer&& other) = default;
  ~DetachedBuffer() = default;

  operator std::string_view() const;

 private:
  flatbuffers::DetachedBuffer buffer_;
};

template <typename T>
  requires std::is_same_v<T, std::pmr::string> ||
           std::is_same_v<T, DetachedBuffer>
struct WriteOps {
  std::optional<std::pmr::string> del_key;
  std::optional<std::pair<std::pmr::string, T>> put_kv;
};

class Serde {
 public:
  template <typename T>
  void Write(this auto&& self,
             TxnBase* txn,
             CFIndex cf_index,
             const std::optional<T>& orig,
             const std::optional<T>& mod);

 private:
  template <typename T>
  auto GetWriteOps(this auto&& self,
                   const std::optional<T>& orig,
                   const std::optional<T>& mod)
      -> WriteOps<decltype(self.SerVal(*mod))>;
};

template <typename T>
void Serde::Write(this auto&& self,
                  TxnBase* txn,
                  CFIndex cf_index,
                  const std::optional<T>& orig,
                  const std::optional<T>& mod) {
  auto write_ops = self.GetWriteOps(orig, mod);
  if (write_ops.del_key) {
    txn->Del(cf_index, *write_ops.del_key);
  }
  if (write_ops.put_kv) {
    txn->Put(cf_index, write_ops.put_kv->first, write_ops.put_kv->second);
  }
}

template <typename T>
auto Serde::GetWriteOps(this auto&& self,
                        const std::optional<T>& orig,
                        const std::optional<T>& mod)
    -> WriteOps<decltype(self.SerVal(*mod))> {
  if (!mod) {
    if (orig) {
      return {.del_key = self.SerKey(*orig)};
    }
    return {};
  }
  if (!orig) {
    return {.put_kv = std::make_pair(self.SerKey(*mod), self.SerVal(*mod))};
  }
  bool key_changed = self.IsKeyChanged(*orig, *mod);
  bool value_changed = self.IsValueChanged(*orig, *mod);
  return {.del_key = key_changed ? std::make_optional(self.SerKey(*orig))
                                 : std::nullopt,
          .put_kv = (key_changed || value_changed)
                        ? std::make_optional(std::make_pair(self.SerKey(*mod),
                                                            self.SerVal(*mod)))
                        : std::nullopt};
}

class InodeSerde : public Serde {
 public:
  explicit InodeSerde(ReqScopedAlloc alloc);
  InodeSerde(const InodeSerde&) = delete;
  InodeSerde(InodeSerde&&) = delete;
  InodeSerde& operator=(const InodeSerde&) = delete;
  InodeSerde& operator=(InodeSerde&&) = delete;
  ~InodeSerde() = default;

  bool IsKeyChanged(const Dir& orig, const Dir& mod) const;
  bool IsKeyChanged(const File& orig, const File& mod) const;
  bool IsValueChanged(const Dir& orig, const Dir& mod) const;
  bool IsValueChanged(const File& orig, const File& mod) const;

  std::pmr::string SerKey(InodeID id);
  std::pmr::string SerKey(const Dir& dir);
  std::pmr::string SerKey(const File& file);
  DetachedBuffer SerVal(const Dir& dir);
  DetachedBuffer SerVal(const File& file);

  InodeID DeKey(std::string_view key);
  std::variant<Dir, File> DeVal(std::string_view val);

 private:
  ReqScopedAlloc alloc_;
  FBAllocator fb_alloc_;
};

class MTimeSerde : public Serde {
 public:
  explicit MTimeSerde(ReqScopedAlloc alloc);
  MTimeSerde(const MTimeSerde&) = delete;
  MTimeSerde(MTimeSerde&&) = delete;
  MTimeSerde& operator=(const MTimeSerde&) = delete;
  MTimeSerde& operator=(MTimeSerde&&) = delete;

  bool IsKeyChanged(const Dir& orig, const Dir& mod) const;
  bool IsKeyChanged(const File& orig, const File& mod) const;
  bool IsValueChanged(const Dir& orig, const Dir& mod) const;
  bool IsValueChanged(const File& orig, const File& mod) const;

  std::pmr::string SerKey(InodeID id);
  std::pmr::string SerKey(const Dir& dir);
  std::pmr::string SerKey(const File& file);
  std::pmr::string SerVal(int64_t mtime_in_ns);
  std::pmr::string SerVal(const Dir& dir);
  std::pmr::string SerVal(const File& file);

  int64_t DeVal(std::string_view val);

 private:
  ReqScopedAlloc alloc_;
  FBAllocator fb_alloc_;
};

class ATimeSerde : public Serde {
 public:
  explicit ATimeSerde(ReqScopedAlloc alloc);
  ATimeSerde(const ATimeSerde&) = delete;
  ATimeSerde(ATimeSerde&&) = delete;
  ATimeSerde& operator=(const ATimeSerde&) = delete;
  ATimeSerde& operator=(ATimeSerde&&) = delete;
  ~ATimeSerde() = default;

  bool IsKeyChanged(const Dir& orig, const Dir& mod) const;
  bool IsKeyChanged(const File& orig, const File& mod) const;
  bool IsValueChanged(const Dir& orig, const Dir& mod) const;
  bool IsValueChanged(const File& orig, const File& mod) const;

  std::pmr::string SerKey(InodeID id);
  std::pmr::string SerKey(const Dir& dir);
  std::pmr::string SerKey(const File& file);
  std::pmr::string SerVal(int64_t atime_in_ns);
  std::pmr::string SerVal(const Dir& dir);
  std::pmr::string SerVal(const File& file);

  int64_t DeVal(std::string_view val);

 private:
  ReqScopedAlloc alloc_;
  FBAllocator fb_alloc_;
};

class DEntSerde : public Serde {
 public:
  explicit DEntSerde(ReqScopedAlloc alloc);
  DEntSerde(const DEntSerde&) = delete;
  DEntSerde(DEntSerde&&) = delete;
  DEntSerde& operator=(const DEntSerde&) = delete;
  DEntSerde& operator=(DEntSerde&&) = delete;
  ~DEntSerde() = default;

  bool IsKeyChanged(const Dir& orig, const Dir& mod) const;
  bool IsKeyChanged(const HardLink& orig, const HardLink& mod) const;
  bool IsValueChanged(const Dir& orig, const Dir& mod) const;
  bool IsValueChanged(const HardLink& orig, const HardLink& mod) const;

  std::pmr::string SerKey(InodeID parent_id, std::string_view name);
  std::pmr::string SerKey(const Dir& dir);
  std::pmr::string SerKey(const HardLink& file);
  DetachedBuffer SerVal(const Dir& dir);
  DetachedBuffer SerVal(const HardLink& hard_link);

  std::tuple<InodeID, std::pmr::string> DeKey(std::string_view key);
  std::variant<Dir, HardLink> DeVal(std::string_view val);

 private:
  ReqScopedAlloc alloc_;
  FBAllocator fb_alloc_;
};

}  // namespace rocketfs
