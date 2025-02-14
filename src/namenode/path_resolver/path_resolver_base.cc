// Copyright 2025 RocketFS

#include "namenode/path_resolver/path_resolver_base.h"

namespace rocketfs {

// Refer to the move constructor of `vector`. If the allocators are the same,
// simply swap the internal data.  Otherwise, use `std::__uninitialized_move_a`
// to move the internal data to the new storage.
// https://github.com/gcc-mirror/gcc/blob/2f33fa09aabf1e8825c7a3cb553054be32b7320c/libstdc%2B%2B-v3/include/bits/stl_vector.h#L666
PathComponent::PathComponent(allocator_type allocator)
    : allocator_(allocator),
      full_path_(allocator),
      last_inode_name_(allocator) {
}

}  // namespace rocketfs
