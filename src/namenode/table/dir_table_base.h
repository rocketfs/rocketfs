// Copyright 2025 RocketFS

#pragma once

#include <sys/stat.h>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <variant>

#include <unifex/task.hpp>

#include "common/status.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

// POSIX permissions consist of two parts:
// 1. Classes
//    An inode has three classes: user class, group class, and others class.
//    When checking permissions, a req includes a `uid` and `gid`:
//    - If `uid` matches the inode's `uid`, user class permissions apply.
//    - If `gid` matches the inode's `gid`, group class permissions apply.
//    - Otherwise, others class permissions apply.
// 2. Permissions
//    Each class has three types of permissions: read, write, and execute.
//    - For regular files: Permissions are straightforward.
//    - For directories: Permissions are often misunderstood:
//      - The read perm allows listing the names of files in the directory
//        but does not provide further information, such as file contents, file
//        type, size, ownership, or permissions.
//      - The write perm allows modifying entries in the directory, such
//        as creating, deleting, or renaming files. However, this requires the
//        execute perm to be set; without it, the write perm is
//        meaningless for directories.
//      - The execute perm is interpreted as the "search" perm for
//        directories. It grants access to file contents and meta-information if
//        the file name is known. However, it does not allow listing files
//        inside the directory unless the read perm is also set.
// Reference: https://en.wikipedia.org/wiki/File-system_permissions
struct Acl {
  uint32_t uid;
  uint32_t gid;
  uint64_t perm;
};

inline bool operator==(const Acl& lhs, const Acl& rhs) {
  return lhs.uid == rhs.uid && lhs.gid == rhs.gid && lhs.perm == rhs.perm;
}

struct User {
  uint32_t uid;
  uint32_t gid;
};

inline std::expected<void, Status> CheckPermission(const Acl& acl,
                                                   const User& user,
                                                   uint64_t perm) {
  uint64_t perm_needed = 0;
  if (user.uid == 0) {
    perm_needed = 0;
  } else if (user.uid == acl.uid) {
    static_assert(S_IRWXU == S_IRWXO << 6);
    perm_needed = perm << 6;
  } else if (user.gid == acl.gid) {
    static_assert(S_IRWXG == S_IRWXO << 3);
    perm_needed = perm << 3;
  } else {
    perm_needed = perm;
  }
  if ((acl.perm & perm_needed) != perm_needed) {
    return std::unexpected(Status::PermissionError(
        fmt::format("User {{uid: {}, gid: {}}} does not have permission for {} "
                    "in ACL {{uid: {}, gid: {}, perm: {:04o}}}.",
                    user.uid,
                    user.gid,
                    perm,
                    acl.uid,
                    acl.gid,
                    acl.perm)));
  }
  return {};
}

struct Dir {
  InodeID parent_id;
  std::pmr::string name;
  InodeID id;
  Acl acl;
  int64_t ctime_in_ns;
  int64_t mtime_in_ns;
  int64_t atime_in_ns;
};

// CREATE TABLE DirTable (
//   parent_id INTEGER NOT NULL,
//   name VARCHAR(255) NOT NULL,
//   id INTEGER NOT NULL,
//   attr TEXT NOT NULL,
//   PRIMARY KEY (id),
//   UNIQUE (parent_id, name),
//   INDEX (parent_id, name),
//   FOREIGN KEY (parent_id) REFERENCES DirTable(id)
// );
class DirTableBase {
 public:
  DirTableBase() = default;
  DirTableBase(const DirTableBase&) = delete;
  DirTableBase(DirTableBase&&) = delete;
  DirTableBase& operator=(const DirTableBase&) = delete;
  DirTableBase& operator=(DirTableBase&&) = delete;
  virtual ~DirTableBase() = default;

  virtual unifex::task<std::expected<std::optional<Dir>, Status>> Read(
      InodeID id) = 0;
  virtual unifex::task<std::expected<std::optional<Dir>, Status>> Read(
      InodeID parent_id, std::string_view name) = 0;
  virtual void Write(const std::optional<Dir>& original,
                     const std::optional<Dir>& modified) = 0;
};

}  // namespace rocketfs
