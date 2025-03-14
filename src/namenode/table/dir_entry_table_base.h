// Copyright 2025 RocketFS

#pragma once

#include <string>

#include "namenode/table/inode_table_base.h"

namespace rocketfs {

class DirEntry {
  friend class DirEntryT;

 public:
  explicit DirEntry(std::pmr::string dir_entry_str);
  DirEntry(const DirEntry&) = delete;
  DirEntry(DirEntry&&) = default;
  DirEntry& operator=(const DirEntry&) = delete;
  DirEntry& operator=(DirEntry&&) = default;
  ~DirEntry() = default;

  InodeID parent_inode_id() const;
  std::string_view name() const;
  InodeID inode_id() const;

 private:
  std::pmr::string dir_entry_str_;
  const serde::DirEntry* dir_entry_;
};

class DirEntryT {
 public:
  DirEntryT() = default;
  explicit DirEntryT(const DirEntry& dir_entry);
  DirEntryT(const DirEntryT&) = default;
  DirEntryT(DirEntryT&&) = default;
  DirEntryT& operator=(const DirEntryT&) = default;
  DirEntryT& operator=(DirEntryT&&) = default;
  ~DirEntryT() = default;

  InodeID parent_inode_id() const;
  std::string_view name() const;
  InodeID inode_id() const;

  InodeID& parent_inode_id();
  std::string& name();
  InodeID& inode_id();

 private:
  serde::DirEntryT dir_entry_;
};

class ResolvedPaths;
class DirEntryTableBase {
 public:
  DirEntryTableBase() = default;
  DirEntryTableBase(const DirEntryTableBase&) = delete;
  DirEntryTableBase(DirEntryTableBase&&) = delete;
  DirEntryTableBase& operator=(const DirEntryTableBase&) = delete;
  DirEntryTableBase& operator=(DirEntryTableBase&&) = delete;
  virtual ~DirEntryTableBase() = default;

  virtual std::expected<std::variant<std::monostate, Inode, InodeID>, Status>
  Read(InodeID parent_inode_id, std::string_view name) = 0;
  virtual void Write(const ResolvedPaths& resolved_paths) = 0;
};

}  // namespace rocketfs
