// Copyright 2025 RocketFS

#include <gflags/gflags.h>

namespace rocketfs {

DEFINE_uint32(request_monotonic_buffer_resource_prealloc_bytes,
              4096,
              "Preallocate memory (in bytes) for the monotonic buffer resource "
              "used by a request.");

}  // namespace rocketfs
