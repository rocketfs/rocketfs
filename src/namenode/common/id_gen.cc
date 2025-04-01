// Copyright 2025 RocketFS

#include "namenode/common/id_gen.h"

#include <chrono>
#include <memory>

#include "common/time_util.h"

namespace rocketfs {

IDGenMgr::IDGenMgr(TimeUtilBase* time_util) : time_util_(time_util) {
  CHECK_NOTNULL(time_util_);
}

void IDGenMgr::Start() {
  CHECK_NULL(thread_);
  thread_ = std::make_unique<std::thread>([this]() {
    while (true) {
      for (auto id_generator : id_generators_) {
        auto current_timestamp_seconds =
            time_util_->NowNs() / (kMsToUs * kUsToNs);
        CHECK_GT(current_timestamp_seconds, kEpochTimestampSeconds);
        current_timestamp_seconds -= kEpochTimestampSeconds;
        id_generator->Update(current_timestamp_seconds);
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
}

void IDGenMgr::Stop() {
  CHECK_NOTNULL(thread_);
  thread_->join();
  thread_ = nullptr;
  id_generators_.clear();
}

}  // namespace rocketfs
