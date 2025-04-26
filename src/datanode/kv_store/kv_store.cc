// Copyright 2025 RocketFS

#include "datanode/kv_store/kv_store.h"

#include <spdk/bdev.h>
#include <spdk/blob.h>
#include <spdk/blob_bdev.h>
#include <spdk/env.h>
#include <spdk/event.h>
#include <spdk/log.h>
#include <spdk/stdinc.h>
#include <spdk/string.h>

namespace rocketfs {

KVStore::KVStore(std::string_view json_conf_filepath) {
  struct spdk_app_opts opts;
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "kv_store";
  opts.json_config_file = json_conf_filepath.data();
  opts.rpc_addr = nullptr;
  spdk_app_start(&opts, kv_store_main, nullptr);
}

}  // namespace rocketfs
