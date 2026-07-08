#pragma once
#include <source_location>
#include "Core/Timer.hpp"

namespace axm::profiler {

    class ScopedTimer
    {
    public:
        ScopedTimer(std::source_location loc = std::source_location::current());
        ~ScopedTimer();
    };

}
