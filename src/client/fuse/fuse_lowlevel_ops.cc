// Copyright 2025 RocketFS

#include "client/fuse/fuse_lowlevel_ops.h"

#include <fuse3/fuse_lowlevel.h>
#include <quill/core/ThreadContextManager.h>
#include <stddef.h>
#include <sys/types.h>

#include <memory>

#include "client/fuse/fuse_ops_proxy.h"
#include "client/fuse/operation/fuse_get_attr_operation.h"
#include "client/fuse/operation/fuse_lookup_operation.h"
#include "client/fuse/operation/fuse_open_dir_operation.h"
#include "client/fuse/operation/fuse_read_dir_operation.h"
#include "client/fuse/operation/fuse_release_dir_operation.h"

namespace rocketfs {

const struct fuse_lowlevel_ops gFuseLowlevelOps = {
    .init =
        [](void* userdata, struct fuse_conn_info* conn) {
          gFuseOpsProxy->Init(userdata, conn);
        },
    .destroy = [](void* userdata) { gFuseOpsProxy->Destroy(userdata); },

    .lookup =
        [](fuse_req_t req, fuse_ino_t parent, const char* name) {
          gFuseOpsProxy->CreateOperation<FuseLookupOperation>(
              req, parent, name);
        },
    .getattr =
        [](fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi) {
          gFuseOpsProxy->CreateOperation<FuseGetAttrOperation>(req, ino, fi);
        },
    .opendir =
        [](fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi) {
          gFuseOpsProxy->CreateOperation<FuseOpenDirOperation>(req, ino, fi);
        },
    .readdir =
        [](fuse_req_t req,
           fuse_ino_t ino,
           size_t size,
           off_t off,
           struct fuse_file_info* fi) {
          gFuseOpsProxy->CreateOperation<FuseReadDirOperation>(
              req, ino, size, off, fi);
        },
    .releasedir =
        [](fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi) {
          gFuseOpsProxy->CreateOperation<FuseReleaseDirOperation>(req, ino, fi);
        },
};

}  // namespace rocketfs
