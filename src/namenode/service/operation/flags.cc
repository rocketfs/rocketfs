// Copyright 2025 RocketFS

#include <gflags/gflags.h>

namespace rocketfs {

DEFINE_uint32(list_dir_default_limit,
              100,
              "The max num of entries to return in a single list dir op.");

}  // namespace rocketfs
