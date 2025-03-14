// Copyright 2025 RocketFS

#pragma once

#include <optional>
#include <utility>
#include <variant>

#include "common/logger.h"

namespace rocketfs {

template <typename T, typename R>
class Mutation {
 public:
  explicit Mutation(std::optional<T>&& original);
  Mutation(const Mutation&) = delete;
  Mutation(Mutation&&) = default;
  Mutation& operator=(const Mutation&) = delete;
  Mutation& operator=(Mutation&&) = default;
  ~Mutation() = default;

  operator bool() const;

  void Create();
  void Delete();

  R* operator->();
  R& operator*();

  const R* operator->() const;
  const R& operator*() const;

  const std::optional<T>& GetOriginal() const;
  const std::variant<std::monostate, std::optional<R>>& GetModified() const;

 private:
  std::optional<T> original_;
  std::variant<std::monostate, std::optional<R>> modified_;
};

template <typename T, typename R>
Mutation<T, R>::Mutation(std::optional<T>&& original)
    : original_(std::forward<std::optional<T>>(original)),
      modified_(original_.and_then(
          [](const auto& t) { return std::make_optional(R(t)); })) {
}

template <typename T, typename R>
Mutation<T, R>::operator bool() const {
  return modified_.index() == 0 ? original_ != std::nullopt
                                : static_cast<bool>(std::get<1>(modified_));
}

template <typename T, typename R>
void Mutation<T, R>::Create() {
  CHECK(!*this);
  modified_ = R();
}

template <typename T, typename R>
void Mutation<T, R>::Delete() {
  CHECK(*this);
  modified_ = std::nullopt;
}

template <typename T, typename R>
R* Mutation<T, R>::operator->() {
  CHECK(*this);
  return &*std::get<1>(modified_);
}

template <typename T, typename R>
R& Mutation<T, R>::operator*() {
  return *operator->();
}

template <typename T, typename R>
const R* Mutation<T, R>::operator->() const {
  CHECK(*this);
  return &*std::get<1>(modified_);
}

template <typename T, typename R>
const R& Mutation<T, R>::operator*() const {
  return *operator->();
}

template <typename T, typename R>
const std::optional<T>& Mutation<T, R>::GetOriginal() const {
  return original_;
}

template <typename T, typename R>
const std::variant<std::monostate, std::optional<R>>&
Mutation<T, R>::GetModified() const {
  return modified_;
}

}  // namespace rocketfs
