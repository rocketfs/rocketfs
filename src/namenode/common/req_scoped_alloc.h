// Copyright 2025 RocketFS

#pragma once

#include <cstddef>
#include <memory_resource>

namespace rocketfs {

using ReqScopedAlloc = std::pmr::polymorphic_allocator<std::byte>;

}  // namespace rocketfs
