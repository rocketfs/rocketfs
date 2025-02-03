#pragma once

#include <memory>
#include <memory_resource>
#include <string_view>

#include "common/status.h"
#include "generated/inode_generated.h"
#include "namenode/kv_store/kv_store_base.h"

namespace rocketfs {

struct INodeID {
  int64_t index{-1};
};

inline bool operator==(const INodeID& lhs, const INodeID& rhs) {
  return lhs.index == rhs.index;
}

inline bool operator!=(const INodeID& lhs, const INodeID& rhs) {
  return !(lhs == rhs);
}

constexpr INodeID kInvalidINodeID{-1};
constexpr INodeID kRootINodeID{0};

class INode {
 public:
  INode(std::pmr::string basic_info_str,
        std::pmr::string timestamps_and_txids_str);
  INode(const INode&) = delete;
  INode(INode&&) = default;
  INode& operator=(const INode&) = delete;
  INode& operator=(INode&&) = default;
  ~INode() = default;

  // Please keep the field order consistent with inode.fbs.
  int64_t parent_inode_id() const;
  std::string_view name() const;
  int64_t inode_id() const;
  int64_t ctime_in_nanoseconds() const;
  int64_t mtime_in_nanoseconds() const;
  int64_t atime_in_nanoseconds() const;
  int64_t created_txid() const;
  int64_t renamed_txid() const;

 private:
  std::pmr::string basic_info_str_;
  const INodeBasicInfo* basic_info_;
  std::pmr::string timestamps_and_txids_str_;
  const INodeTimestampsAndTxIDs* timestamps_and_txids_;
};

class INodeT {
 public:
  explicit INodeT(const INode& inode);
  INodeT(const INodeT&) = default;
  INodeT(INodeT&&) = default;
  INodeT& operator=(const INodeT&) = default;
  INodeT& operator=(INodeT&&) = default;
  ~INodeT() = default;

  // Please keep the field order consistent with inode.fbs.
  int64_t& parent_inode_id();
  std::string& name();
  int64_t& inode_id();
  int64_t& ctime_in_nanoseconds();
  int64_t& mtime_in_nanoseconds();
  int64_t& atime_in_nanoseconds();
  int64_t& created_txid();
  int64_t& renamed_txid();

 private:
  INodeBasicInfoT basic_info_;
  INodeTimestampsAndTxIDsT timestamps_and_txids_;
};

class INodeTableBase {
 public:
  INodeTableBase() = default;
  INodeTableBase(const INodeTableBase&) = delete;
  INodeTableBase(INodeTableBase&&) = delete;
  INodeTableBase& operator=(const INodeTableBase&) = delete;
  INodeTableBase& operator=(INodeTableBase&&) = delete;
  virtual ~INodeTableBase() = default;

  virtual Status Read(SnapshotBase* snapshot,
                      INodeID parent_inode_id,
                      std::string_view name,
                      INode* inode) = 0;
};

}  // namespace rocketfs
