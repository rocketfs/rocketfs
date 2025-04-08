// Copyright 2025 RocketFS

#pragma once

#include <cstdint>
#include <limits>

#include "namenode/common/id_gen.h"

namespace rocketfs {

struct BlockID {
  int64_t val{std::numeric_limits<int64_t>::max()};
};

inline bool operator==(const BlockID& lhs, const BlockID& rhs) {
  return lhs.val == rhs.val;
}

inline bool operator!=(const BlockID& lhs, const BlockID& rhs) {
  return !(lhs == rhs);
}

constexpr BlockID kInvalidBlockID{std::numeric_limits<int64_t>::max()};

using BlockIDGen = IDGen<BlockID>;

}  // namespace rocketfs
