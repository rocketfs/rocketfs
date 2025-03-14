// Copyright 2025 RocketFS

#include "common/logger.h"

#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/core/LogLevel.h>
#include <quill/sinks/ConsoleSink.h>

#include <string>

namespace rocketfs {

quill::Logger* logger = []() {
  quill::Backend::start();
  auto logger = quill::Frontend::create_or_get_logger(
      "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink"));
  logger->set_log_level(quill::LogLevel::Debug);
  return logger;
}();

}  // namespace rocketfs
