#include "namenode/transaction_manager/table/inode_table_base.h"

#include "common/logger.h"

namespace rocketfs {

INode::INode(std::pmr::string basic_info_str,
             std::pmr::string timestamps_and_txids_str)
    : basic_info_str_(std::move(basic_info_str)),
      timestamps_and_txids_str_(std::move(timestamps_and_txids_str)) {
  std::span<const uint8_t> basic_info_span(
      reinterpret_cast<uint8_t*>(basic_info_str_.data()),
      basic_info_str_.size());
  CHECK(flatbuffers::Verifier(basic_info_span.data(), basic_info_span.size())
            .VerifyBuffer<INodeBasicInfo>());
  basic_info_ = flatbuffers::GetRoot<INodeBasicInfo>(basic_info_span.data());
  CHECK_NOTNULL(basic_info_);

  std::span<const uint8_t> timestamps_and_txids_span(
      reinterpret_cast<uint8_t*>(timestamps_and_txids_str_.data()),
      timestamps_and_txids_str_.size());
  CHECK(flatbuffers::Verifier(timestamps_and_txids_span.data(),
                              timestamps_and_txids_span.size())
            .VerifyBuffer<INodeTimestampsAndTxIDs>());
  timestamps_and_txids_ = flatbuffers::GetRoot<INodeTimestampsAndTxIDs>(
      timestamps_and_txids_span.data());
  CHECK_NOTNULL(timestamps_and_txids_);
}

int64_t INode::parent_inode_id() const {
  return basic_info_->parent_inode_id();
}

std::string_view INode::name() const {
  return {basic_info_->name()->c_str(), basic_info_->name()->size()};
}

int64_t INode::inode_id() const {
  return basic_info_->inode_id();
}

int64_t INode::ctime_in_nanoseconds() const {
  return timestamps_and_txids_->ctime_in_nanoseconds();
}

int64_t INode::mtime_in_nanoseconds() const {
  return timestamps_and_txids_->mtime_in_nanoseconds();
}

int64_t INode::atime_in_nanoseconds() const {
  return timestamps_and_txids_->atime_in_nanoseconds();
}

int64_t INode::created_txid() const {
  return timestamps_and_txids_->created_txid();
}

int64_t INode::renamed_txid() const {
  return timestamps_and_txids_->renamed_txid();
}

INodeT::INodeT(const INode& inode) {
  basic_info_.parent_inode_id = inode.parent_inode_id();
  basic_info_.name = inode.name();
  basic_info_.inode_id = inode.inode_id();
  timestamps_and_txids_.ctime_in_nanoseconds = inode.ctime_in_nanoseconds();
  timestamps_and_txids_.mtime_in_nanoseconds = inode.mtime_in_nanoseconds();
  timestamps_and_txids_.atime_in_nanoseconds = inode.atime_in_nanoseconds();
  timestamps_and_txids_.created_txid = inode.created_txid();
  timestamps_and_txids_.renamed_txid = inode.renamed_txid();
}

int64_t& INodeT::parent_inode_id() {
  return basic_info_.parent_inode_id;
}

std::string& INodeT::name() {
  return basic_info_.name;
}

int64_t& INodeT::inode_id() {
  return basic_info_.inode_id;
}

int64_t& INodeT::ctime_in_nanoseconds() {
  return timestamps_and_txids_.ctime_in_nanoseconds;
}

int64_t& INodeT::mtime_in_nanoseconds() {
  return timestamps_and_txids_.mtime_in_nanoseconds;
}

int64_t& INodeT::atime_in_nanoseconds() {
  return timestamps_and_txids_.atime_in_nanoseconds;
}

int64_t& INodeT::created_txid() {
  return timestamps_and_txids_.created_txid;
}

int64_t& INodeT::renamed_txid() {
  return timestamps_and_txids_.renamed_txid;
}

}  // namespace rocketfs
