// Copyright 2025 RocketFS

#pragma once

#include <cstdint>
#include <expected>
#include <optional>

#include "common/status.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

struct File {
  InodeID id;
  Acl acl;
  int64_t ctime_in_ns;
  int64_t mtime_in_ns;
  int64_t atime_in_ns;
};

// CREATE TABLE FileTable (
//   id INTEGER NOT NULL,
//   attr TEXT NOT NULL,
//   PRIMARY KEY (id)
// );
class FileTableBase {
 public:
  FileTableBase() = default;
  FileTableBase(const FileTableBase&) = delete;
  FileTableBase(FileTableBase&&) = delete;
  FileTableBase& operator=(const FileTableBase&) = delete;
  FileTableBase& operator=(FileTableBase&&) = delete;
  virtual ~FileTableBase() = default;

  virtual std::expected<std::optional<File>, Status> Read(InodeID id) = 0;
  virtual void Write(const std::optional<File>& original,
                     const std::optional<File>& modified) = 0;
};

}  // namespace rocketfs
