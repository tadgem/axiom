#pragma once
#include <chrono>
#include "Prim.hpp"

namespace axm {
    typedef std::chrono::high_resolution_clock HighResolutionClock;

    class Timer
    {
    public:
        typedef HighResolutionClock::time_point TimePoint;

        TimePoint m_Start;

        Timer() : m_Start(HighResolutionClock::now()) { }

        auto Elapsed() { return HighResolutionClock::now() - m_Start; }

        i64 ElapsedMilliseconds() {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Elapsed());
            return static_cast<i64>(ms.count());
        }

        i64 ElapsedNanoseconds() {
            auto ms = std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
            return static_cast<i64>(ms.count());
        }

        f64 ElapsedMillisecondsF() {

            auto ns_raw = std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
            auto ns_f = static_cast<f64>(ns_raw.count());
            return ns_f / 1000000.0;
        }

        f64 ElapsedNanosecondsF() {

            auto ns_raw = std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
            auto ns_f = static_cast<f64>(ns_raw.count());
            return ns_f;
        }
    };
}