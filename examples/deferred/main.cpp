#include <array>
#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Profile.hpp"
#include "Render/ImGuiUtils.hpp"
#include "axiom.hpp"


AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

static aml::Mat44                 g_MVP;
static Transform                  g_Transform;
static Camera                     g_Cam;
static rhi::ComPtr<rhi::ISampler> g_Sampler;

static aml::Mat44                 GetMVP(const Transform& trans, const Camera& cam) {
    return cam.GetViewProjectionMatrix() * trans.GetModelMatrix();
}

namespace {
    struct Drawable
    {
        AssetHandle m_TextureAsset;
        Texture*    m_Texture = nullptr;
        Mesh        m_Mesh;
    };

    void DrawDrawable(GPU&                       gpu,
                      rhi::IRenderPassEncoder*   encoder,
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
    const Timer initTimer = { };

    AxiomEngine init      = engine::Init();
    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    init.m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(init.m_GPU);
    init.m_AssetManager.AddAssetFactory<AssetType::Model, ModelAssetFactory>(init.m_GPU);

    g_MVP                  = GetMVP(g_Transform, g_Cam);

    auto posNormalUvLayout = vertex::PosNormalUV::GetInputLayout();
    posNormalUvLayout.BuildDeviceLayout(init.m_GPU.m_Device);

    Shader cube
            = Shader(init.m_GPU.m_Device, "resources/shaders/cube", Array<String, 2> { "vertexMain", "fragmentMain" });

    Array formats  = { init.m_GPU.m_Surface->getInfo().preferredFormat };

    auto  pipeline = pipeline::CreateRasterPipeline(
            init.m_GPU.m_Device, formats, init.m_GPU.m_DepthStencilDesc, cube, posNormalUvLayout.m_DeviceInputLayout);

    g_Sampler = textures::CreateSampler(
            init.m_GPU.m_Device, rhi::TextureFilteringMode::Linear, rhi::TextureAddressingMode::Wrap);


    f64 msInitTime = initTimer.ElapsedMillisecondsF();

    AXM_LOG("Init took {} ms", msInitTime);
    AXM_LOG("Starting Axiom Main Loop");

    auto drawables = DynArray<Drawable> { };

    init.m_AssetManager.LoadAsset("resources/models/sponza/Sponza.gltf", AssetType::Model, [&drawables](Asset* asset) {
        const auto* model = dynamic_cast<ModelAsset*>(asset);

        for (const auto& entry: model->m_Data.m_Meshes) {
            const auto map = model->m_Data.m_Materials[entry.m_MaterialIndex].m_TextureMaps[TextureMapType::Diffuse];
            drawables.push_back({ .m_TextureAsset = map.m_Handle, .m_Texture = nullptr, .m_Mesh = entry.m_Mesh });
        }
    });

    while (init.m_Running) {
        if (engine::PreFrame(init)) {
            auto viewport              = viewports::GetFullscreenViewport(init.m_Window.m_Window);
            g_Cam.m_ViewportDimensions = viewport.m_Size;
            g_MVP                      = GetMVP(g_Transform, g_Cam);

            auto commandEncoder        = init.m_GPU.m_Queue->createCommandEncoder();
            auto renderPassEncoder     = render_pass::BeginSwapChainRenderPass(
                    init, commandEncoder, rhi::LoadOp::Clear, rhi::LoadOp::Clear, true);
            auto shader = ShaderDataInterface(renderPassEncoder->bindPipeline(pipeline), pipeline->getDesc().label);
            for (auto& drawable: drawables) {

                if (drawable.m_Texture == nullptr) {
                    if (const auto asset = init.m_AssetManager.GetAsset(drawable.m_TextureAsset)) {
                        drawable.m_Texture = &dynamic_cast<TextureAsset*>(asset)->m_Data;
                    }
                }
                DrawDrawable(init.m_GPU, renderPassEncoder, shader, drawable, viewport);
            }

            renderPassEncoder->end();
            init.m_GPU.m_Queue->submit(commandEncoder->finish());
        }

        profiler::ProfilerImGuiWindow(init);

        if (ImGui::Begin("Example (Deferred)")) {
            ImGuiEx::TransformEdit(g_Transform);
            ImGuiEx::CameraEdit(g_Cam);
        }
        ImGui::End();

        engine::PostFrame(init);
    }

    engine::Quit(init);
}
