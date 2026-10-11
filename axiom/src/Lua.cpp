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
    engine_table["vg"] = (void*)engine.m_VK->m_NanoVG;

    state.new_usertype<aml::Float2>("vec2",
        "x", &aml::Float2::x,
        "y", &aml::Float2::y
    );

    state.new_usertype<aml::Vec3>("vec3",
        sol::call_constructor, sol::factories([](const f32 v) {return aml::Vec3(v, v, v); }),
        sol::call_constructor, sol::factories([](const f32 x, const f32 y, const f32 z) {return aml::Vec3(x, y,z); }),
        "x", sol::property(&aml::Vec3::GetX, &aml::Vec3::SetX),
        "y", sol::property(&aml::Vec3::GetY, &aml::Vec3::SetY),
        "z", sol::property(&aml::Vec3::GetZ, &aml::Vec3::SetZ),
        "normalize", &aml::Vec3::Normalized,
        "length", &aml::Vec3::Length,
        sol::meta_function::addition, &aml::Vec3::operator+,
        sol::meta_function::subtraction, [](const aml::Vec3& l, const aml::Vec3& r) {return l - r; },
        sol::meta_function::multiplication, [] (const aml::Vec3& l, const aml::Vec3& r) {return l * r; },
        sol::meta_function::division, [] (const aml::Vec3& l, const aml::Vec3& r) {return l / r; }

    );



}
