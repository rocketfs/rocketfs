// Copyright 2025 RocketFS

#pragma once

#include <cstdint>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string>

#include "common/status.h"
#include "generated/inode_generated.h"
#include "namenode/common/id_gen.h"

namespace rocketfs {

struct HardLink {
  InodeID parent_id;
  std::pmr::string name;
  InodeID id;
};

// CREATE TABLE HardLinkTable (
//   parent_id INTEGER NOT NULL,
//   name VARCHAR(255) NOT NULL,
//   id INTEGER NOT NULL,
//   PRIMARY KEY (parent_id, name),
//   FOREIGN KEY (parent_id, name) REFERENCES DirTable(parent_id,
//   name), FOREIGN KEY (id) REFERENCES FileTable(id)
// );
class HardLinkTableBase {
 public:
  HardLinkTableBase() = default;
  HardLinkTableBase(const HardLinkTableBase&) = delete;
  HardLinkTableBase(HardLinkTableBase&&) = delete;
  HardLinkTableBase& operator=(const HardLinkTableBase&) = delete;
  HardLinkTableBase& operator=(HardLinkTableBase&&) = delete;
  virtual ~HardLinkTableBase() = default;

  virtual std::expected<std::optional<HardLink>, Status> Read(
      InodeID parent_id, std::string_view name) = 0;
  virtual void Write(const std::optional<HardLink>& original,
                     const std::optional<HardLink>& modified) = 0;
};

}  // namespace rocketfs
