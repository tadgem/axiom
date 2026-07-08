#pragma once
#include <source_location>

#include "Core/STL.hpp"
#include "Core/Timer.hpp"
#include "Engine.hpp"

namespace axm {

    struct AppState;
    namespace profiler {

        class ScopedTimer
        {
        public:
            Timer m_Timer;
            const char* m_Label;

            ScopedTimer(const char* label);
            // ScopedTimer(std::source_location loc = std::source_location::current());
            ~ScopedTimer();
        };

        struct ProfilerItem
        {
            f64 m_MeanDuration;
            f64 m_MinDuration = DBL_MAX;
            f64 m_MaxDuration;
        };

        // TODO:  This needs to be reworked when we start doing work across threads.
        // ... but until we do that, we dont have the necessary info to do this correctly
        // (need thread ID to identify which bin to place in, plus a way to find entries if stored as array)
        static HashMap<const char*, ProfilerItem> g_ProfilerItems;

        void ProfilerImGuiWindow(const AppState& e);
    }
}
#ifdef __FUNCTION__
#define PROFILE_SCOPE() axm::profiler::ScopedTimer __FUNCTION__timer = axm::profiler::ScopedTimer(__FUNCTION__)
#else
#define PROFILE_SCOPE() axm::profiler::ScopedTimer __func__timer = axm::profiler::ScopedTimer(__func__)
#endif
