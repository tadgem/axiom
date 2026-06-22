#include "axiom.hpp"
#include <array>
using namespace axm;

int main() {
    AppState init = engine::Init();

    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    mat4 model  = maths::Multiply(
        maths::RotateX(maths::Radians(0.016f)),
        maths::RotateY(maths::Radians(0.016f))
    );
    mat4 view   = maths::Translate(vec3 { 0.0f, 0.0f, -2.5f });
    mat4 proj   = maths::PerspectiveFOV(maths::Radians(45.0f), 1.666f, 0.1f, 10.0f);

    auto modelView     = maths::Multiply(view, model);
    auto mvp    = maths::Multiply(proj, modelView);

    Assimp::Importer importer;
    flecs::world ecs;

    std::array<rhi::Format, 1> formats =
    {
        init.m_SwapchainColourImage->getDesc().format
    };

    Shader cube = Shader(init.m_Device, "resources/cube");
    auto pipeline = pipeline::CreateRasterPipeline(init.m_Device, formats, init.m_DepthStencilDesc, cube, axm::vertex::PosNormalUV::GetInputLayout(init.m_Device));

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
