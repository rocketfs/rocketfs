// Copyright 2025 RocketFS

#include "namenode/transaction_manager/table/inode_table_base.h"

#include <flatbuffers/buffer.h>
#include <flatbuffers/string.h>
#include <flatbuffers/verifier.h>

#include <span>
#include <string>
#include <utility>

#include "common/logger.h"

namespace rocketfs {

INode::INode(std::pmr::string basic_info_str, std::pmr::string timestamps_str)
    : basic_info_str_(std::move(basic_info_str)),
      timestamps_str_(std::move(timestamps_str)) {
  std::span<const uint8_t> basic_info_span(
      reinterpret_cast<uint8_t*>(basic_info_str_.data()),
      basic_info_str_.size());
  CHECK(flatbuffers::Verifier(basic_info_span.data(), basic_info_span.size())
            .VerifyBuffer<INodeBasicInfo>());
  basic_info_ = flatbuffers::GetRoot<INodeBasicInfo>(basic_info_span.data());
  CHECK_NOTNULL(basic_info_);

  std::span<const uint8_t> timestamps_span(
      reinterpret_cast<uint8_t*>(timestamps_str_.data()),
      timestamps_str_.size());
  CHECK(flatbuffers::Verifier(timestamps_span.data(), timestamps_span.size())
            .VerifyBuffer<INodeTimestamps>());
  timestamps_ = flatbuffers::GetRoot<INodeTimestamps>(timestamps_span.data());
  CHECK_NOTNULL(timestamps_);
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

int64_t INode::created_txid() const {
  return basic_info_->created_txid();
}

int64_t INode::renamed_txid() const {
  return basic_info_->renamed_txid();
}

int64_t INode::ctime_in_nanoseconds() const {
  return timestamps_->ctime_in_nanoseconds();
}

int64_t INode::mtime_in_nanoseconds() const {
  return timestamps_->mtime_in_nanoseconds();
}

int64_t INode::atime_in_nanoseconds() const {
  return timestamps_->atime_in_nanoseconds();
}

INodeT::INodeT(const INode& inode) {
  inode.basic_info_->UnPackTo(&basic_info_);
  inode.timestamps_->UnPackTo(&timestamps_);
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

int64_t& INodeT::created_txid() {
  return basic_info_.created_txid;
}

int64_t& INodeT::renamed_txid() {
  return basic_info_.renamed_txid;
}

int64_t& INodeT::ctime_in_nanoseconds() {
  return timestamps_.ctime_in_nanoseconds;
}

int64_t& INodeT::mtime_in_nanoseconds() {
  return timestamps_.mtime_in_nanoseconds;
}

int64_t& INodeT::atime_in_nanoseconds() {
  return timestamps_.atime_in_nanoseconds;
}

}  // namespace rocketfs
