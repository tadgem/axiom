#include <print>
#include "Axiom.hpp"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_slang_rhi.h"

using namespace axm;

int main() {
    std::println("AXIOM");
    AppState init = engine::Init();

    mat4 model  = maths::Multiply(
        maths::RotateX(maths::Radians(0.016f)),
        maths::RotateY(maths::Radians(0.016f))
    );
    mat4 view   = maths::Translate(vec3 { 0.0f, 0.0f, -2.5f });
    mat4 proj   = maths::PerspectiveFOV(maths::Radians(45.0f), 1.666f, 0.1f, 10.0f);

    auto modelView     = maths::Multiply(view, model);
    auto mvp    = maths::Multiply(proj, modelView);


    while (init.m_Running) {
        engine::PreFrame(init);

        if (ImGui::Begin("Hello!")) {

        }
        ImGui::End();
        ImGui::Render();



        engine::PostFrame(init);
    }


    engine::Quit(init);

}
