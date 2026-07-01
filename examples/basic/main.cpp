#include "axiom.hpp"
#include <array>

AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;


mat4 GetMVP(const vec3& pos, const vec3& euler, const vec3& scale) {
    mat4 model  = maths::GetModelMatrix(pos, euler, scale);
    mat4 view   = maths::Translate(vec3{0.0f, 0.0f, 0.0f});
    mat4 proj   = maths::PerspectiveFOV(maths::Radians(45.0f), 1.666f, 0.1f, 100.0f);
    auto modelView     = maths::Multiply(view, model);
    return maths::Multiply(proj, modelView);
}


int main() {

    constexpr u32 width = 1280;
    constexpr u32 height = 720;

    Timer initTimer = {};

    AppState init = engine::Init();

    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    vec3 position = {};
    vec3 euler = {};
    vec3 scale = {1.0, 1.0, 1.0};
    auto mvp    = GetMVP(position, euler, scale);

    auto posNormalUvLayout = vertex::PosNormalUV::GetInputLayout(init.m_Device);
    auto cubeShapeDef = shapes::GetCubeShape();
    auto cubeMesh = meshes::CreateMeshFromData(
        init.m_Device,
        cubeShapeDef.m_VertexBuffer,
        cubeShapeDef.m_BufferLength * sizeof(f32),
        cubeShapeDef.m_IndexBuffer,
        cubeShapeDef.m_NumIndices * sizeof(u32),
        posNormalUvLayout
    );

    Shader cube = Shader(init.m_Device, "resources/shaders/cube");

    Array formats = {
        init.m_Surface->getInfo().preferredFormat
    };

    auto pipeline = pipeline::CreateRasterPipeline(
        init.m_Device,
        formats,
        init.m_DepthStencilDesc,
        cube,
        posNormalUvLayout
    );

    auto cpuTextureData = textures::LoadCPUTextureDataFromFile("resources/textures/crate.jpg");

    auto gpuTexture = textures::CreateTexture2D(
        init.m_Device,
        cpuTextureData.m_Data,
        rhi::Format::RGBA8Unorm,
        cpuTextureData.m_Width, cpuTextureData.m_Height
    );

    auto sampler = textures::CreateSampler(
        init.m_Device,
        rhi::TextureFilteringMode::Linear,
        rhi::TextureAddressingMode::ClampToEdge
    );

    cpuTextureData.Release();

    f64 msInitTime = initTimer.ElapsedMillisecondsF();

    AXM_LOG("Init took {} ms", msInitTime);
    AXM_LOG("Starting Axiom Main Loop");
    AXM_FLUSH_LOG();

    auto viewport = viewports::GetFullscreenViewport(init.m_Window);


    while (init.m_Running) {
        engine::PreFrame(init);
        mvp = GetMVP(position, euler, scale);

        auto commandEncoder = init.m_Queue->createCommandEncoder();
        auto renderPassEncoder = render_pass::BeginSwapChainRenderPass(init, commandEncoder);

        auto cursor = rhi::ShaderCursor(renderPassEncoder->bindPipeline(pipeline));
        if (SLANG_FAILED(cursor["modelViewProj"].setData(&mvp, sizeof(mat4)))) {
            AXM_LOG("Failed to bind modelViewProj to pipeline");
        }

        if (SLANG_FAILED(cursor["diffuse"].setBinding(gpuTexture.m_TextureView))) {
            AXM_LOG("Failed to bind diffuse texture to pipeline");
        }

        if (SLANG_FAILED(cursor["diffuseSampler"].setBinding(sampler))) {
            AXM_LOG("Failed to bind diffuse sampler to pipeline");
        }

        meshes ::DrawMesh(viewport, cubeMesh, renderPassEncoder);


        if (ImGui::Begin("Hello!")) {
            ImGui::DragFloat3("Position", &position.x);
            ImGui::DragFloat3("Euler", &euler.x);
            ImGui::DragFloat3("Scale", &scale.x);
        }
        ImGui::End();
        ImGui::Render();

        renderPassEncoder->end();
        init.m_Queue->submit(commandEncoder->finish());

        engine::PostFrame(init);
    }

    engine::Quit(init);

}
