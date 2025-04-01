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

#include "common/logger.h"

namespace rocketfs {

enum class StatusCode : int16_t {
  kSystemError = 1,

  kInvalidArgumentError = 2,
  kPermissionError = 3,
  kAlreadyExistsError = 4,
  kNotFoundError = 5,
  kNotDirError = 6,
  kParentNotFoundError = 7,
  kParentNotDirError = 8,

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
  inline static Status SystemError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status InvalidArgumentError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status PermissionError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status AlreadyExistsError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status NotFoundError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status NotDirError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status ParentNotFoundError(
      std::string_view msg = "",
      const std::optional<Status>& internal_error = std::nullopt,
      std::source_location location = std::source_location::current());
  inline static Status ParentNotDirError(
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

  inline std::string_view GetMsg() const;

  template <typename Resp>
  Resp MakeError();

 private:
  StatusCode status_code_;
  std::string msg_;
};

Status Status::SystemError(std::string_view msg,
                           const std::optional<Status>& internal_error,
                           std::source_location location) {
  return Status(StatusCode::kSystemError, msg, internal_error, location);
}

Status Status::InvalidArgumentError(std::string_view msg,
                                    const std::optional<Status>& internal_error,
                                    std::source_location location) {
  return Status(
      StatusCode::kInvalidArgumentError, msg, internal_error, location);
}

Status Status::PermissionError(std::string_view msg,
                               const std::optional<Status>& internal_error,
                               std::source_location location) {
  return Status(StatusCode::kPermissionError, msg, internal_error, location);
}

Status Status::AlreadyExistsError(std::string_view msg,
                                  const std::optional<Status>& internal_error,
                                  std::source_location location) {
  return Status(StatusCode::kAlreadyExistsError, msg, internal_error, location);
}

Status Status::NotFoundError(std::string_view msg,
                             const std::optional<Status>& internal_error,
                             std::source_location location) {
  return Status(StatusCode::kNotFoundError, msg, internal_error, location);
}

Status Status::NotDirError(std::string_view msg,
                           const std::optional<Status>& internal_error,
                           std::source_location location) {
  return Status(StatusCode::kNotDirError, msg, internal_error, location);
}

Status Status::ParentNotFoundError(std::string_view msg,
                                   const std::optional<Status>& internal_error,
                                   std::source_location location) {
  return Status(
      StatusCode::kParentNotFoundError, msg, internal_error, location);
}

Status Status::ParentNotDirError(std::string_view msg,
                                 const std::optional<Status>& internal_error,
                                 std::source_location location) {
  return Status(StatusCode::kParentNotDirError, msg, internal_error, location);
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
      // Error 1 at src/main.cpp:10: high-level failure.
      // Caused by: Error 2 at src/file.cpp:123: operation failed.
      // Caused by: Error 3 at src/other_file.cpp:45: invalid input.
      msg_(fmt::format(
          // e.g., "Error 1 at src/main.cpp:10".
          "Error {} at location {}:{}"
          // e.g., ": high-level failure.".
          "{}"
          // e.g., "Caused by: Error 2 at src/file.cpp:123: operation failed.".
          "{}",
          // e.g., "Error 1 at src/main.cpp:10".
          static_cast<int>(status_code),
          location.file_name(),
          location.line(),
          // e.g., ": high-level failure.".
          msg.empty() ? "." : fmt::format(": {}", msg),
          // e.g., "Caused by: Error 2 at src/file.cpp:123: operation failed.".
          !internal_error
              ? ""
              : fmt::format(" Caused by: {}", internal_error->GetMsg()))) {
  CHECK(msg.empty() || msg.back() == '.');
  CHECK_EQ(msg_.back(), '.');
}

std::string_view Status::GetMsg() const {
  return msg_;
}

template <typename Resp>
Resp Status::MakeError() {
  Resp resp;
  resp.set_error_code(static_cast<int>(status_code_));
  resp.set_error_msg(std::move(msg_));
  return resp;
}

}  // namespace rocketfs
