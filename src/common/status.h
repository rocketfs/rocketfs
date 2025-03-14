#pragma once

#include <fmt/core.h>

#include <atomic>
#include <compare>
#include <iterator>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#include "namenode/kv_store/column_family.h"

namespace rocketfs {

enum class StatusCode : int16_t {
  kInvalidArgumentError = 1,
  kNotFoundError = 2,

  kPermissionError = 3,
  kNotDirError = 4,

  // Error encountered in `KVStoreBase`.
  kConflictError = 1002,
};

class Status {
 private:
  inline Status(StatusCode status_code,
                std::string_view msg,
                const std::optional<Status>& internal_error,
                std::source_location location);

 public:
  inline static Status InvalidArgumentError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status NotFoundError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status ConflictError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());

  Status(const Status&) = default;
  Status(Status&&) = default;
  Status& operator=(const Status&) = default;
  Status& operator=(Status&&) = default;
  ~Status() = default;

  inline bool IsInvalidArgumentError() const;
  inline bool IsNotFoundError() const;

  inline std::string_view GetMsg() const;

 private:
  StatusCode status_code_;
  std::string msg_;
};

Status Status::InvalidArgumentError(std::string_view msg,
                                    const std::optional<Status>& internal_error,
                                    std::source_location location) {
  return Status(
      StatusCode::kInvalidArgumentError, msg, internal_error, location);
}

Status Status::NotFoundError(std::string_view msg,
                             const std::optional<Status>& internal_error,
                             std::source_location location) {
  return Status(StatusCode::kNotFoundError, msg, internal_error, location);
}

Status Status::ConflictError(std::string_view msg,
                             const std::optional<Status>& internal_error,
                             std::source_location location) {
  return Status(StatusCode::kConflictError, msg, internal_error, location);
}

Status::Status(StatusCode status_code,
               std::string_view msg,
               const std::optional<Status>& internal_error,
               std::source_location location)
    : status_code_(status_code),
      // Example of an error message structure:
      // Error at src/main.cpp:10: high-level failure.
      // Caused by: Error at src/file.cpp:123: operation failed.
      // Caused by: Error at src/other_file.cpp:45: invalid input.
      msg_(fmt::format("Error {} at location {}:{}{}.{}",
                       static_cast<int>(status_code),
                       location.file_name(),
                       location.line(),
                       msg.empty() ? "" : fmt::format(": {}", msg),
                       internal_error ? fmt::format(" Caused by: {}",
                                                    internal_error->GetMsg())
                                      : "")) {
}

std::string_view Status::GetMsg() const {
  return msg_;
}

bool Status::IsNotFoundError() const {
  return status_code_ == StatusCode::kNotFoundError;
}

}  // namespace rocketfs
