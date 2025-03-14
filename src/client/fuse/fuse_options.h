// Copyright 2025 RocketFS

#pragma once

#include <string>

namespace rocketfs {

struct FuseOptions {
  std::string server_endpoint;
  std::string remote_mountpoint;
  int rpc_timeout_ms;
  // The `fuse_entry_param` structure includes a `generation` number. However,
  // RocketFS's inode does not track generations, so the generation is defined
  // by the FUSE client.
  // https://github.com/libfuse/libfuse/blob/master/include/fuse_lowlevel.h#L70
  int entry_generation{0};
  double attr_timeout_sec{0.0};
  double entry_timeout_sec{0.0};
};

}  // namespace rocketfs
