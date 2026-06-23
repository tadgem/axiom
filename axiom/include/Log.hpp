#pragma once
#include <print>

#define AXM_LOG(fmt, ...) std::println("Axiom : " fmt, __VA_ARGS__)
;
#define AXM_FLUSH_LOG() std::fflush(stdout)

#define AXM_ASSERT(X, msg) assert(X && msg)