// Copyright 2025 RocketFS

#pragma once

#define FUSE_USE_VERSION 312

#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>

namespace rocketfs {

extern const struct fuse_lowlevel_ops gFuseLowlevelOps;

}  // namespace rocketfs
