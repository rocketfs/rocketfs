// Copyright 2025 RocketFS

#define FUSE_USE_VERSION 312

#ifdef linux
// For `pread`/`pwrite`/`utimensat`.
#define _XOPEN_SOURCE 700
#endif

#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <fuse3/fuse_opt.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "client/fuse/fuse_lowlevel_ops.h"
#include "client/fuse/fuse_ops_proxy.h"
#include "client/fuse/fuse_options.h"
#include "common/defer.h"
#include "common/logger.h"

namespace rocketfs {}  // namespace rocketfs

int main(int argc, char* argv[]) {
  // ./fuse /tmp/rocketfs -f -s
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  DEFER([&]() { fuse_opt_free_args(&args); });
  struct fuse_cmdline_opts opts;
  CHECK_EQ(fuse_parse_cmdline(&args, &opts), 0);
  CHECK(opts.singlethread && opts.foreground);
  CHECK_NOTNULL(opts.mountpoint);
  DEFER([&]() { free(opts.mountpoint); });
  rocketfs::FuseOptions fuse_options{
      .server_endpoint = "localhost:50051",
      .remote_mountpoint = "/",
      .rpc_timeout_ms = 1000,
      .entry_generation = 0,
      .attr_timeout_sec = 0.0,
      .entry_timeout_sec = 0.0,
  };
  rocketfs::gFuseOpsProxy =
      std::make_unique<rocketfs::FuseOpsProxy>(fuse_options);

  auto session =
      CHECK_NOTNULL(fuse_session_new(&args,
                                     &rocketfs::gFuseLowlevelOps,
                                     sizeof(rocketfs::gFuseLowlevelOps),
                                     nullptr));
  DEFER([&]() { fuse_session_destroy(session); });
  CHECK_EQ(fuse_set_signal_handlers(session), 0);
  DEFER([&]() { fuse_remove_signal_handlers(session); });
  CHECK_EQ(fuse_session_mount(session, opts.mountpoint), 0);
  DEFER([&]() { fuse_session_unmount(session); });
  CHECK_EQ(fuse_daemonize(true), 0);
  CHECK_EQ(fuse_session_loop(session), 0);
  return 0;
}
