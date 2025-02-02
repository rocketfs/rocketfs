#pragma once

namespace rocketfs {

class NameNode {
 public:
  NameNode();
  NameNode(const NameNode&) = delete;
  NameNode(NameNode&&) = delete;
  NameNode& operator=(const NameNode&) = delete;
  NameNode& operator=(NameNode&&) = delete;
  ~NameNode();
};

}  // namespace rocketfs
