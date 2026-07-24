#include <array>
#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Profile.hpp"
#include "axiom.hpp"


AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

static aml::Mat44                 g_MVP;
static rhi::ComPtr<rhi::ISampler> g_Sampler;


static aml::Mat44                 GetMVP(const aml::Vec3& pos, const aml::Vec3& euler, const aml::Vec3& scale) {
    const auto model  = Utils::CreateModelMatrix(pos, euler, scale);
    const auto camPos = aml::Vec3(0.0f, 0.0f, 0.0f);
    const auto view   = aml::Mat44::sTranslation(camPos);
    const auto proj = aml::Mat44::sPerspective(aml::DegreesToRadians(60.0f), 1280.0 / 720.0, 0.03f, 10000.0f);
    return proj * view * model;
}
namespace {
    struct Drawable
    {
        AssetHandle m_TextureAsset;
        Texture*    m_Texture = nullptr;
        Mesh        m_Mesh;
    };

    void DrawDrawable(rhi::IRenderPassEncoder*   encoder,
                      const ShaderDataInterface& shader,
                      const Drawable&            drawable,
                      const Viewport&            viewport) {

        shader.SetData("modelViewProj", g_MVP);
        if (drawable.m_Texture) {
            shader.SetBinding("diffuse", drawable.m_Texture->m_TextureView);
        }
        shader.SetBinding("diffuseSampler", g_Sampler);

        meshes ::DrawMesh(viewport, drawable.m_Mesh, encoder);
    }
}
int main() {

    constexpr u32 width     = 1280;
    constexpr u32 height    = 720;

    const Timer   initTimer = { };

    AppState      init      = engine::Init();
    init.m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(init.m_GPU);
    init.m_AssetManager.AddAssetFactory<AssetType::Model, ModelAssetFactory>(init.m_GPU);
    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    aml::Vec3 position     = { 0.0f, 0.0f, 0.0f };
    aml::Vec3 euler        = { 0.0f, 0.0f, 0.0f };
    aml::Vec3 scale        = aml::Vec3(0.16f, 0.16f, 0.16f);
    g_MVP                  = GetMVP(position, euler, scale);

    auto posNormalUvLayout = vertex::PosNormalUV::GetInputLayout();
    posNormalUvLayout.BuildDeviceLayout(init.m_GPU.m_Device);

    Array<String, 2> entries  = { "vertexMain", "fragmentMain" };
    Shader           cube     = Shader(init.m_GPU.m_Device, "resources/shaders/cube", entries);

    Array            formats  = { init.m_GPU.m_Surface->getInfo().preferredFormat };

    auto             pipeline = pipeline::CreateRasterPipeline(
            init.m_GPU.m_Device, formats, init.m_DepthStencilDesc, cube, posNormalUvLayout.m_DeviceInputLayout);

    g_Sampler = textures::CreateSampler(
            init.m_GPU.m_Device, rhi::TextureFilteringMode::Linear, rhi::TextureAddressingMode::Wrap);


    f64 msInitTime = initTimer.ElapsedMillisecondsF();

    AXM_LOG("Init took {} ms", msInitTime);
    AXM_LOG("Starting Axiom Main Loop");
    AXM_FLUSH_LOG();

    auto viewport  = viewports::GetFullscreenViewport(init.m_Window);

    auto drawables = DynArray<Drawable> { };

    init.m_AssetManager.LoadAsset("resources/models/sponza/Sponza.gltf", AssetType::Model, [&drawables](Asset* asset) {
        const auto* model = dynamic_cast<ModelAsset*>(asset);

        for (const auto& entry: model->m_Data.m_Meshes) {
            const auto map = model->m_Data.m_Materials[entry.m_MaterialIndex].m_TextureMaps[TextureMapType::Diffuse];
            drawables.push_back({ .m_TextureAsset = map.m_Handle, .m_Texture = nullptr, .m_Mesh = entry.m_Mesh });
        }
    });

    while (init.m_Running) {
        engine::PreFrame(init);
        g_MVP                  = GetMVP(position, euler, scale);

        auto commandEncoder    = init.m_GPU.m_Queue->createCommandEncoder();
        auto renderPassEncoder = render_pass::BeginSwapChainRenderPass(
                init, commandEncoder, rhi::LoadOp::Clear, rhi::LoadOp::Clear, true);
        auto shader = ShaderDataInterface(renderPassEncoder->bindPipeline(pipeline), pipeline->getDesc().label);
        for (auto& drawable: drawables) {

            if (drawable.m_Texture == nullptr) {
                if (const auto asset = init.m_AssetManager.GetAsset(drawable.m_TextureAsset)) {
                    drawable.m_Texture = &dynamic_cast<TextureAsset*>(asset)->m_Data;
                }
            }
            DrawDrawable(renderPassEncoder, shader, drawable, viewport);
        }

        renderPassEncoder->end();
        init.m_GPU.m_Queue->submit(commandEncoder->finish());

        profiler::ProfilerImGuiWindow(init);

        if (ImGui::Begin("Hello!")) {
            ImGui::DragFloat3("Position", &position.mF32[0]);
            ImGui::DragFloat3("Euler", &euler.mF32[0]);
            ImGui::DragFloat3("Scale", &scale.mF32[0]);
        }
        ImGui::End();

        engine::PostFrame(init);
    }

    engine::Quit(init);
}
