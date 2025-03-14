// Copyright 2025 RocketFS

#pragma once

#include <cstdint>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string>

#include "common/status.h"
#include "generated/inode_generated.h"
#include "namenode/common/id_generator.h"

namespace rocketfs {

struct InodeID {
  int64_t index{-1};
};

inline bool operator==(const InodeID& lhs, const InodeID& rhs) {
  return lhs.index == rhs.index;
}

inline bool operator!=(const InodeID& lhs, const InodeID& rhs) {
  return !(lhs == rhs);
}

constexpr InodeID kInvalidInodeID{-1};
constexpr InodeID kRootInodeID{0};

using InodeIDGenerator = IDGenerator<InodeID>;

class Inode {
  friend class InodeT;

 public:
  Inode(std::pmr::string basic_info_str, std::pmr::string timestamps_str);
  Inode(const Inode&) = delete;
  Inode(Inode&&) = default;
  Inode& operator=(const Inode&) = delete;
  Inode& operator=(Inode&&) = default;
  ~Inode() = default;

  // Please keep the field order consistent with inode.fbs.
  InodeID inode_id() const;
  serde::InodeType inode_type() const;
  const serde::File* as_file() const;
  const serde::Directory* as_directory() const;
  const serde::Symlink* as_symlink() const;
  int64_t created_txid() const;
  int64_t renamed_txid() const;
  int64_t ctime_in_nanoseconds() const;
  int64_t mtime_in_nanoseconds() const;
  int64_t atime_in_nanoseconds() const;

  const serde::InodeBasicInfo& basic_info() const;
  const serde::InodeTimestamps& timestamps() const;

 private:
  std::pmr::string basic_info_str_;
  const serde::InodeBasicInfo* basic_info_;
  std::pmr::string timestamps_str_;
  const serde::InodeTimestamps* timestamps_;
};

class InodeT {
 public:
  InodeT() = default;
  explicit InodeT(const Inode& Inode);
  InodeT(const InodeT&) = default;
  InodeT(InodeT&&) = default;
  InodeT& operator=(const InodeT&) = default;
  InodeT& operator=(InodeT&&) = default;
  ~InodeT() = default;

  // Please keep the field order consistent with inode.fbs.
  InodeID& inode_id();
  serde::InodeTypeUnion& inode_type_union();
  int64_t& created_txid();
  int64_t& renamed_txid();
  int64_t& ctime_in_nanoseconds();
  int64_t& mtime_in_nanoseconds();
  int64_t& atime_in_nanoseconds();

  InodeID inode_id() const;
  serde::InodeType inode_type() const;
  const serde::FileT* as_file() const;
  const serde::DirectoryT* as_directory() const;
  const serde::SymlinkT* as_symlink() const;
  int64_t created_txid() const;
  int64_t renamed_txid() const;
  int64_t ctime_in_nanoseconds() const;
  int64_t mtime_in_nanoseconds() const;
  int64_t atime_in_nanoseconds() const;

  const serde::InodeBasicInfoT& basic_info() const;
  const serde::InodeTimestampsT& timestamps() const;

 private:
  serde::InodeBasicInfoT basic_info_;
  serde::InodeTimestampsT timestamps_;
};

class ResolvedPaths;
class InodeTableBase {
 public:
  InodeTableBase() = default;
  InodeTableBase(const InodeTableBase&) = delete;
  InodeTableBase(InodeTableBase&&) = delete;
  InodeTableBase& operator=(const InodeTableBase&) = delete;
  InodeTableBase& operator=(InodeTableBase&&) = delete;
  virtual ~InodeTableBase() = default;

  virtual std::expected<std::optional<Inode>, Status> Read(
      InodeID inode_id) = 0;
  virtual void Write(const ResolvedPaths& resolved_paths) = 0;
};

}  // namespace rocketfs
