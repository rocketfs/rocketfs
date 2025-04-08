// Copyright 2025 RocketFS

#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <vector>

#include "common/status.h"
#include "namenode/table/block_id.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

struct Block {
  BlockID id;
  int64_t off;
  int64_t len;
};

struct File {
  InodeID id;
  Acl acl;
  int32_t nlink;
  int64_t len;
  int32_t block_size;
  std::vector<Block> blocks;
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

  virtual unifex::task<std::expected<std::optional<File>, Status>> Read(
      InodeID id) = 0;
  virtual void Write(const std::optional<File>& orig,
                     const std::optional<File>& mod) = 0;
};

}  // namespace rocketfs
