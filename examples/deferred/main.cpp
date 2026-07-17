#include <array>
#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Profile.hpp"
#include "axiom.hpp"

AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

static mat4                       g_MVP;
static rhi::ComPtr<rhi::ISampler> g_Sampler;

mat4                              GetMVP(const vec3& pos, const vec3& euler, const vec3& scale) {
    const auto model = maths::GetModelMatrix(pos, euler, scale);
    const auto view  = maths::Translate(vec3 { 0.0f, 0.0f, 0.0f });
    const auto proj  = maths::PerspectiveFOV(maths::Radians(45.0f), 1.666f, 0.1f, 100.0f);
    const auto modelView = maths::Multiply(view, model);
    return maths::Multiply(proj, modelView);
}

struct Drawable
{
    AssetHandle m_TextureAsset;
    Texture*    m_Texture = nullptr;
    Mesh        m_Mesh;
};

void DrawDrawble(rhi::IRenderPassEncoder*                 encoder,
                 const rhi::ComPtr<rhi::IRenderPipeline>& pipeline,
                 const Drawable&                          drawable,
                 const Viewport&                          viewport) {
    auto cursor = ShaderDataInterface(encoder, pipeline);

    cursor.SetData("modelViewProj", g_MVP);
    if (drawable.m_Texture) {
        cursor.SetBinding("diffuse", drawable.m_Texture->m_TextureView);
    }
    cursor.SetBinding("diffuseSampler", g_Sampler);

    meshes ::DrawMesh(viewport, drawable.m_Mesh, encoder);
}

int main() {

    constexpr u32 width     = 1280;
    constexpr u32 height    = 720;

    Timer         initTimer = { };

    AppState      init      = engine::Init();
    init.m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(init.m_Device);
    init.m_AssetManager.AddAssetFactory<AssetType::Model, ModelAssetFactory>(init.m_Device);
    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    vec3 position          = { };
    vec3 euler             = { };
    vec3 scale             = { 1.0, 1.0, 1.0 };
    g_MVP                  = GetMVP(position, euler, scale);

    auto posNormalUvLayout = vertex::PosNormalUV::GetInputLayout();
    posNormalUvLayout.BuildDeviceLayout(init.m_Device);

    Shader cube     = Shader(init.m_Device, "resources/shaders/cube");

    Array  formats  = { init.m_Surface->getInfo().preferredFormat };

    auto   pipeline = pipeline::CreateRasterPipeline(
            init.m_Device, formats, init.m_DepthStencilDesc, cube, posNormalUvLayout.m_DeviceInputLayout);

    g_Sampler = textures::CreateSampler(
            init.m_Device, rhi::TextureFilteringMode::Linear, rhi::TextureAddressingMode::ClampToEdge);


    f64 msInitTime = initTimer.ElapsedMillisecondsF();

    AXM_LOG("Init took {} ms", msInitTime);
    AXM_LOG("Starting Axiom Main Loop");
    AXM_FLUSH_LOG();

    auto viewport  = viewports::GetFullscreenViewport(init.m_Window);

    auto drawables = Vector<Drawable> { };

    init.m_AssetManager.LoadAsset("resources/models/sponza/Sponza.gltf", AssetType::Model, [&drawables](Asset* asset) {
        ModelAsset* model = static_cast<ModelAsset*>(asset);

        for (auto& entry: model->m_Data.m_Meshes) {
            auto map = model->m_Data.m_Materials[entry.m_MaterialIndex].m_TextureMaps[TextureMapType::Diffuse];
            drawables.push_back({ .m_TextureAsset = map.m_Handle, .m_Texture = nullptr, .m_Mesh = entry.m_Mesh });
        }
    });

    while (init.m_Running) {
        engine::PreFrame(init);
        g_MVP                  = GetMVP(position, euler, scale);

        auto commandEncoder    = init.m_Queue->createCommandEncoder();
        auto renderPassEncoder = render_pass::BeginSwapChainRenderPass(init, commandEncoder);

        for (auto& drawable: drawables) {

            if (drawable.m_Texture == nullptr) {
                if (auto asset = init.m_AssetManager.GetAsset(drawable.m_TextureAsset)) {
                    drawable.m_Texture = &static_cast<TextureAsset*>(asset)->m_Data;
                }
            }
            DrawDrawble(renderPassEncoder, pipeline, drawable, viewport);
        }

        renderPassEncoder->end();
        init.m_Queue->submit(commandEncoder->finish());

        profiler::ProfilerImGuiWindow(init);

        if (ImGui::Begin("Hello!")) {
            ImGui::DragFloat3("Position", &position.x);
            ImGui::DragFloat3("Euler", &euler.x);
            ImGui::DragFloat3("Scale", &scale.x);
        }
        ImGui::End();

        engine::PostFrame(init);
    }

    engine::Quit(init);
}
