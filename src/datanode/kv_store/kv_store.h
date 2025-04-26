// Copyright 2025 RocketFS

#pragma once

#include <string_view>

namespace rocketfs {

class KVStore {
 public:
  explicit KVStore(std::string_view json_conf_filepath);

 private:
};

}  // namespace rocketfs
