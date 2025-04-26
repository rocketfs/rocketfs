// Copyright 2025 RocketFS

#include <functional>

#include <unifex/create.hpp>

void thirdFunc(int i, const std::function<void(int j)>& callback) {
  callback(i + 1);
}

int main(int argc, char** argv) {
  return 0;
}
