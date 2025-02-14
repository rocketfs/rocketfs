// Copyright 2025 RocketFS

#include <gflags/gflags.h>

namespace rocketfs {

DEFINE_string(rocksdb_kv_store_db_path,
              "/tmp/rocksdb",
              "The path for the RocksDB KVStore database.");

}  // namespace rocketfs
