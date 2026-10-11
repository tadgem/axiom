#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"

namespace axm {
	struct AxiomEngine;

	namespace lua {
		void* LuaCustomAllocator(void* ud, void* ptr, size_t osize, size_t nsize);
		sol::state	CreateLuaState(AxiomEngine& engine);
		void		InitLuaAPI(sol::state& state, AxiomEngine& engine);
	}
}