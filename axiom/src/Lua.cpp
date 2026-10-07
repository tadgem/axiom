#include "Script/Lua.hpp"
#include "mimalloc.h"
void* axm::Lua::LuaCustomAllocator(void* ud, void* ptr, size_t osize, size_t nsize){

    // 1. Free memory
    if (nsize == 0) {
        if (ptr != NULL) {
            mi_free(ptr);
        }
        return NULL;
    }

    // 2. Allocate or reallocate memory
    return mi_realloc(ptr, nsize);
}

sol::state axm::Lua::CreateLuaState()
{
    return std::move(sol::state(sol::default_at_panic, axm::Lua::LuaCustomAllocator, nullptr));
}
