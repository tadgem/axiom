#pragma once
#include <cassert>
#include <print>
#include <stacktrace>

#define KILOBYTES(X) X * 1024ULL
#define MEGABYTES(X) X * 1024ULL * 1024ULL
#define GIGABYTES(X) X * 1024ULL * 1024ULL * 1024ULL

#define NORMAL_PRINT_CODE "\x1B[0m"
#define RED_PRINT_CODE "\x1B[31m"
#define GREEN_PRINT_CODE "\x1B[32m"
#define YELLOW_PRINT_CODE "\x1B[33m"
#define BLUE_PRINT_CODE "\x1B[34m"
#define MAGENTA_PRINT_CODE "\x1B[35m"
#define CYAN_PRINT_CODE "\x1B[36m"
#define WHITE_PRINT_CODE "\x1B[37m"

// TODO (LiamD) : This should be configured from cmake.
#define AXM_ENABLE_LOGGING
#ifdef AXM_ENABLE_LOGGING
#define AXM_LOG(fmt, ...) std::println("Axiom : " fmt, __VA_ARGS__)
#define AXM_LOG_INFO(fmt, ...) std::println("Axiom : INFO : " fmt, __VA_ARGS__)
#define AXM_LOG_ERROR(fmt, ...) std::println("Axiom : ERROR : " fmt, __VA_ARGS__)
#define AXM_FLUSH_LOG() std::fflush(stdout)
#else
#define AXM_LOG(fmt, ...)
#define AXM_LOG_INFO(fmt, ...)
#define AXM_LOG_ERROR(fmt, ...)
#define AXM_FLUSH_LOG()
#endif

#define AXM_ASSERT(X, msg) assert(X &&msg)
#define AXM_ASSERT_NOT_REACHED() AXM_ASSERT(false, "Should not be reached")

#define STACK_TRACE() AXM_LOG("TRACE:\n{}\n", std::to_string(std::stacktrace::current()))
