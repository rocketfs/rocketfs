// Copyright 2025 RocketFS

#pragma once

#include <expected>
#include <string_view>
#include <variant>

#include "common/status.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

// -- The combination of (parent_id, name) serves as a primary key.
// CREATE VIEW DirEntryView AS
// SELECT
//   parent_id,
//   name,
//   id
// FROM
//   DirTable
// UNION ALL
// SELECT
//   parent_id,
//   name,
//   id
// FROM
//   HardLinkTable
class DEntViewBase {
 public:
  DEntViewBase() = default;
  DEntViewBase(const DEntViewBase&) = delete;
  DEntViewBase(DEntViewBase&&) = delete;
  DEntViewBase& operator=(const DEntViewBase&) = delete;
  DEntViewBase& operator=(DEntViewBase&&) = delete;
  virtual ~DEntViewBase() = default;

  virtual unifex::task<
      std::expected<std::variant<std::monostate, Dir, HardLink>, Status>>
  Read(InodeID parent_id, std::string_view name) = 0;

  virtual unifex::task<
      std::expected<std::pmr::vector<std::variant<Dir, HardLink>>, Status>>
  List(InodeID parent_id, std::string_view start_after, size_t limit) = 0;
};

}  // namespace rocketfs
