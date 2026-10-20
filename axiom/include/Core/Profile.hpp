#pragma once
#include <source_location>

#include "Core/STL.hpp"
#include "Core/Timer.hpp"

namespace axm
{

struct AxiomEngine;
namespace profiler
{

	class ScopedTimer
	{
	public:
		Timer       m_Timer;
		const char* m_Label;

		ScopedTimer(const char* label);
		// ScopedTimer(std::source_location loc = std::source_location::current());
		~ScopedTimer();
	};

	struct ProfilerItem
	{
		f64 m_MeanDuration;
		f64 m_ThisFrameDuration;
		f64 m_MinDuration = DBL_MAX;
		f64 m_MaxDuration;
	};

	void ProfilerImGuiWindow(const AxiomEngine& e);
}  // namespace profiler
}  // namespace axm
#ifdef __FUNCTION__
#define PROFILE_SCOPE() axm::profiler::ScopedTimer __FUNCTION__timer = axm::profiler::ScopedTimer(__FUNCTION__);
#else
#define PROFILE_SCOPE() axm::profiler::ScopedTimer __func__timer = axm::profiler::ScopedTimer(__func__);
#endif

#define NAMED_SCOPE(name) axm::profiler::ScopedTimer name##timer = axm::profiler::ScopedTimer(#name);
