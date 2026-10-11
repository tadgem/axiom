#include "Script/Lua.hpp"
#include "mimalloc.h"
#include "Axiom.hpp"
#include "nanovg.h"
void* axm::lua::LuaCustomAllocator(void* ud, void* ptr, size_t osize, size_t nsize){
    // free
    if (nsize == 0) {
        if (ptr != NULL) {
            mi_free(ptr);
        }
        return NULL;
    }

    // allocate or reallocate memory
    return mi_realloc(ptr, nsize);
}

sol::state axm::lua::CreateLuaState(AxiomEngine& engine)
{
    auto state = sol::state(sol::default_at_panic, axm::lua::LuaCustomAllocator, nullptr);
    InitLuaAPI(state, engine);
	state.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::ffi);

    return std::move(state);
}

void axm::lua::InitLuaAPI(sol::state& state, AxiomEngine& engine)
{
    using namespace vku;

    auto engine_table = state.create_named_table("engine");
    engine_table["vg"] = (void*) engine.m_VK->m_NanoVG;
}
