#pragma once

#include <flatbuffers/allocator.h>
#include <flatbuffers/detached_buffer.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "namenode/common/req_scoped_alloc.h"

namespace rocketfs {

struct CFIndex {
  int8_t index{-1};
};

inline bool operator==(const CFIndex& lhs, const CFIndex& rhs) {
  return lhs.index == rhs.index;
}

inline bool operator!=(const CFIndex& lhs, const CFIndex& rhs) {
  return !(lhs == rhs);
}

constexpr CFIndex kInvalidCFIndex{-1};
constexpr CFIndex kDefaultCFIndex{0};
constexpr CFIndex kInodeCFIndex{1};
constexpr CFIndex kMTimeCFIndex{2};
constexpr CFIndex kATimeCFIndex{3};
constexpr CFIndex kDEntCFIndex{4};

}  // namespace rocketfs
