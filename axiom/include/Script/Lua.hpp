#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"

namespace axm::Lua {
	void*		LuaCustomAllocator(void* ud, void* ptr, size_t osize, size_t nsize);
	sol::state	CreateLuaState();


}