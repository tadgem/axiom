#include <array>
#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Profile.hpp"
#include "axiom.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"


AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

static glm::mat4                  g_MVP;
static rhi::ComPtr<rhi::ISampler> g_Sampler;

static mat4                       GetMVP(const vec3& pos, const vec3& euler, const vec3& scale) {
    const auto model     = maths::GetModelMatrix(pos, euler, scale);
    const auto view      = maths::Translate(vec3 { 0.0f, 0.0f, 0.0f });
    const auto proj      = maths::PerspectiveFOV(maths::Radians(45.0f), 1.666f, 0.1f, 100.0f);
    const auto modelView = maths::Multiply(view, model);
    return maths::Multiply(proj, modelView);
}

glm::quat GetQuatFromEuler(glm::vec3 euler) {
    glm::vec3 eulerRadians = glm::vec3(glm::radians(euler.x), glm::radians(euler.y), glm::radians(euler.z));
    glm::quat xRotation    = glm::angleAxis(eulerRadians.x, glm::vec3(1, 0, 0));
    glm::quat yRotation    = glm::angleAxis(eulerRadians.y, glm::vec3(0, 1, 0));
    glm::quat zRotation    = glm::angleAxis(eulerRadians.z, glm::vec3(0, 0, 1));

    return zRotation * yRotation * xRotation;
}

static glm::mat4 GetGlmModel(const glm::vec3& pos, const glm::vec3& euler, const glm::vec3& scale) {
    glm::mat4 modelMatrix   = glm::mat4(1.0);

    modelMatrix             = glm::translate(modelMatrix, pos);
    glm::quat rot           = GetQuatFromEuler(euler);
    glm::mat4 localRotation = glm::mat4_cast(rot);
    glm::mat4 localScale    = glm::mat4(1.0);
    localScale              = glm::scale(localScale, scale);

    return modelMatrix * localRotation * localScale;
}

static glm::mat4 GetGlmMVP(const glm::vec3& pos, const glm::vec3& euler, const glm::vec3& scale) {
    const auto model = GetGlmModel(pos, euler, scale);
    const auto view  = glm::translate(glm::mat4(1.0f), -glm::vec3(0.01f));
    const auto proj  = glm::perspectiveRH_ZO(glm::radians(60.0f), 1280.0f / 720.0f, 0.01f, 1000.0f);
    return proj * view * model;
}

struct Drawable
{
    AssetHandle m_TextureAsset;
    Texture*    m_Texture = nullptr;
    Mesh        m_Mesh;
};

static void DrawDrawble(rhi::IRenderPassEncoder* encoder,
                        ShaderDataInterface&     shader,
                        const Drawable&          drawable,
                        const Viewport&          viewport) {

    shader.SetData("modelViewProj", g_MVP);
    if (drawable.m_Texture) {
        shader.SetBinding("diffuse", drawable.m_Texture->m_TextureView);
    }
    shader.SetBinding("diffuseSampler", g_Sampler);

    meshes ::DrawMesh(viewport, drawable.m_Mesh, encoder);
}

int main() {

    constexpr u32 width     = 1280;
    constexpr u32 height    = 720;

    const Timer   initTimer = { };

    AppState      init      = engine::Init();
    init.m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(init.m_Device);
    init.m_AssetManager.AddAssetFactory<AssetType::Model, ModelAssetFactory>(init.m_Device);
    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    glm::vec3 position     = { 0.0f, 0.0f, 0.0f };
    glm::vec3 euler        = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale        = { 1.0, 1.0, 1.0 };
    g_MVP                  = GetGlmMVP(position, euler, scale);

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
        const auto* model = dynamic_cast<ModelAsset*>(asset);

        for (const auto& entry: model->m_Data.m_Meshes) {
            const auto map = model->m_Data.m_Materials[entry.m_MaterialIndex].m_TextureMaps[TextureMapType::Diffuse];
            drawables.push_back({ .m_TextureAsset = map.m_Handle, .m_Texture = nullptr, .m_Mesh = entry.m_Mesh });
        }
    });

    while (init.m_Running) {
        engine::PreFrame(init);
        g_MVP                  = GetGlmMVP(position, euler, scale);

        auto commandEncoder    = init.m_Queue->createCommandEncoder();
        auto renderPassEncoder = render_pass::BeginSwapChainRenderPass(
                init, commandEncoder, rhi::LoadOp::Clear, rhi::LoadOp::Clear, true);
        auto shader = ShaderDataInterface(renderPassEncoder, pipeline);
        for (auto& drawable: drawables) {

            if (drawable.m_Texture == nullptr) {
                if (const auto asset = init.m_AssetManager.GetAsset(drawable.m_TextureAsset)) {
                    drawable.m_Texture = &dynamic_cast<TextureAsset*>(asset)->m_Data;
                }
            }
            DrawDrawble(renderPassEncoder, shader, drawable, viewport);
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
