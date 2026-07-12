#pragma once
#include <cstdio>
#define FLECS_USE_OS_ALLOC
#include "Assets/Assets.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Debug.hpp"
#include "Core/Engine.hpp"
#include "Core/JSON.hpp"
#include "Core/STL.hpp"
#include "Core/Timer.hpp"
#include "Core/Utils.hpp"

#include "flecs.h"
#include "flecs/addons/cpp/flecs.hpp"

namespace axm {
    enum class TestResultEnum { Fail = -1, Pass = 0, DNF = 1 };

    using TestString = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

    template <typename _Value>
    using TestVector = std::vector<_Value, std::allocator<_Value>>;

    struct TestResult
    {
        TestString        mName;
        TestResultEnum    mResult;
        TestString        mResultMessage;
        f64               mElapsedMs;

        static TestResult Pass() { return { "Unknown Test", TestResultEnum::Pass, { }, 0.0 }; };

        static TestResult Fail(const TestString& reason) {
            return { "Unknown Test", TestResultEnum::Fail, reason, 0.0 };
        };
    };

} // namespace axm

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define FLOAT_ROUGHLY_EQUAL(x, y) abs(abs(x) - abs(y)) <= 0.00001

#define AXM_TEST_ASSERT(cond, message, ...)                                                                            \
    if (!(cond)) {                                                                                                     \
        return TestResult { "",                                                                                        \
                            axm::TestResultEnum::Fail,                                                                 \
                            message " : Test failed at " __FILE__ ", Line " STRINGIZE(__LINE__) " : " #cond, 0.0 };    \
    }

#define AXM_BEGIN_TESTS(suite_name)                                                                                    \
    AXM_OVERRIDE_GLOBAL_NEW(true)                                                                                      \
    int main() {                                                                                                       \
        TestVector<axm::TestResult> sResults { };                                                                      \
        AXM_LOG_INFO("{} Tests", suite_name);

#define AXM_END_TESTS()                                                                                                \
    for (auto& result: sResults) {                                                                                     \
        AXM_LOG_INFO("Test {}, Result : {}" NORMAL_PRINT_CODE ", Time Taken : {} ms",                                  \
                     result.mName.c_str(),                                                                             \
                     result.mResult == axm::TestResultEnum::Fail ? RED_PRINT_CODE "Fail" : GREEN_PRINT_CODE "Pass",    \
                     result.mElapsedMs);                                                                               \
        if (result.mResult != axm::TestResultEnum::Pass) {                                                             \
            AXM_LOG_ERROR("Message : {}", result.mResultMessage.c_str());                                              \
        }                                                                                                              \
    }                                                                                                                  \
    AXM_FLUSH_LOG();                                                                                                   \
    }                                                                                                                  \
    ;

#define AXM_ADD_TEST(TEST_NAME)                                                                                        \
    {                                                                                                                  \
        AXM_LOG_INFO("Running Test : {}", #TEST_NAME);                                                                 \
        AppState   engine = engine::Init();                                                                            \
        axm::Timer timer_##TEST_NAME;                                                                                  \
        auto       result_##TEST_NAME = TEST_NAME(&engine);                                                            \
        result_##TEST_NAME.mName      = #TEST_NAME;                                                                    \
        f64 time_taken_##TEST_NAME    = timer_##TEST_NAME.ElapsedMillisecondsF();                                      \
        result_##TEST_NAME.mElapsedMs = time_taken_##TEST_NAME;                                                        \
        sResults.push_back(result_##TEST_NAME);                                                                        \
        engine::Quit(engine);                                                                                          \
    }
