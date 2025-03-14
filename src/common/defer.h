// Copyright 2025 RocketFS

#pragma once

#include <absl/cleanup/cleanup.h>

#define __DEFER(salt, func) auto defer_##salt = absl::MakeCleanup((func))
#define _DEFER(salt, func) __DEFER(salt, (func))
#define DEFER(func) _DEFER(__LINE__, (func))
