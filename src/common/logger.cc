// Copyright 2025 RocketFS

#include "common/logger.h"

#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

#include <string>

namespace rocketfs {

quill::Logger* logger = []() {
  quill::Backend::start();
  return quill::Frontend::create_or_get_logger(
      "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink"));
}();

}  // namespace rocketfs
