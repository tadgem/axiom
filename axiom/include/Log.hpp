#pragma once
#include <print>

#define AXM_LOG(fmt, ...) std::println(fmt, __VA_ARGS__)

#define AXM_ASSERT(X, msg) assert(X && msg)