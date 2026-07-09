#pragma once
#include <chrono>
#include "Prim.hpp"

namespace axm {
    typedef std::chrono::high_resolution_clock HighResolutionClock;

    class Timer
    {
    public:
        typedef HighResolutionClock::time_point TimePoint;

        TimePoint                               m_Start;

        Timer() : m_Start(HighResolutionClock::now()) { }

        NO_DISCARD auto Elapsed() const { return HighResolutionClock::now() - m_Start; }
        void            Reset() { m_Start = HighResolutionClock::now(); }

        NO_DISCARD i64  ElapsedMilliseconds() const {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Elapsed());
            return static_cast<i64>(ms.count());
        }

        NO_DISCARD i64 ElapsedNanoseconds() const {
            auto ms = std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
            return static_cast<i64>(ms.count());
        }

        NO_DISCARD f64 ElapsedMillisecondsF() const {

            auto ns_raw = std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
            auto ns_f   = static_cast<f64>(ns_raw.count());
            return ns_f / 1000000.0;
        }

        NO_DISCARD f64 ElapsedNanosecondsF() const {

            auto ns_raw = std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
            auto ns_f   = static_cast<f64>(ns_raw.count());
            return ns_f;
        }
    };
}
