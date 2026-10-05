#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Profile.hpp"
#include "ImGui/vku_extensions.h"
#include "Render/ImGuiUtils.hpp"
#include "axiom.hpp"

AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

static aml::Mat44 g_MVP;
static Transform  g_Transform;
static Camera     g_Cam;

static aml::Mat44 GetMVP(const Transform& trans, const Camera& cam) {
    return cam.GetViewProjectionMatrix() * trans.GetModelMatrix();
}

namespace {
    struct Drawable
    {
        AssetHandle   m_TextureAsset;
        Texture*      m_Texture = nullptr;
        Mesh          m_Mesh;
        vku::Material m_Material;
    };
}

int main() {
    const Timer initTimer = { };

    AxiomEngine init = AxiomEngine::Init();
    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    vku::VkState& vk = *init.m_VK;

    init.m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(init.m_GPU);
    init.m_AssetManager.AddAssetFactory<AssetType::Model, ModelAssetFactory>(init.m_GPU);

    g_Transform.m_Scale = aml::Vec3(0.2f, 0.2f, 0.2f);
    g_MVP               = GetMVP(g_Transform, g_Cam);

    vku::VertexDescription vertDesc = vertex::PosNormalUV::GetVertexDescription(vk);

    Shader gbufferShader(vk, "resources/shaders/gbuffer.vert", "resources/shaders/gbuffer.frag");

    // G-Buffer: colour (RGB10A2), normal (RG16F), uv (RG16F) + depth.
    vku::Framebuffer fb(*vk.m_CPUAllocator);
    fb.AddColourAttachment(vk,
                           vku::ResolutionScale::Full,
                           1,
                           VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           VK_SAMPLE_COUNT_1_BIT);
    fb.AddColourAttachment(vk,
                           vku::ResolutionScale::Full,
                           1,
                           VK_FORMAT_R16G16_SFLOAT,
                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           VK_SAMPLE_COUNT_1_BIT);
    fb.AddColourAttachment(vk,
                           vku::ResolutionScale::Full,
                           1,
                           VK_FORMAT_R16G16_SFLOAT,
                           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           VK_SAMPLE_COUNT_1_BIT);
    fb.AddDepthAttachment(vk,
                          vku::ResolutionScale::Full,
                          1,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          VK_SAMPLE_COUNT_1_BIT);

    fb.m_AttachmentLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    fb.m_ClearValues.resize(4);
    for (u32 i = 0; i < 3; i++) {
        fb.m_ClearValues[i].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    }
    fb.m_ClearValues[3].depthStencil = { 1.0f, 0 };
    fb.Build(vk);

    vku::RasterizationState rasterState = vku::defaults::DefaultRasterState;
    vku::VkPipelineData     pipeline    = pipeline::CreateRasterPipeline(vk,
                                                     gbufferShader,
                                                     vertDesc,
                                                     rasterState,
                                                     fb.m_RenderPassInfo.m_RenderPass,
                                                     fb.m_Resolution,
                                                     3);

    const f64 msInitTime = initTimer.ElapsedMillisecondsF();
    AXM_LOG("Init took {} ms", msInitTime);
    AXM_LOG("Starting Axiom Main Loop");

    auto drawables = DynArray<Drawable> { };

    init.m_AssetManager.LoadAsset(
            "resources/models/sponza/Sponza.gltf", AssetType::Model, [&](Asset* asset) {
                const auto* model = dynamic_cast<ModelAsset*>(asset);
                for (const auto& entry: model->m_Data.m_Meshes) {
                    Drawable drawable;
                    drawable.m_Mesh = entry.m_Mesh;
                    if (entry.m_MaterialIndex < model->m_Data.m_Materials.size()) {
                        drawable.m_TextureAsset
                                = model->m_Data.m_Materials[entry.m_MaterialIndex]
                                          .m_TextureMaps[TextureMapType::Diffuse]
                                          .m_Handle;
                    }
                    drawable.m_Material = vku::Material::Create(vk, gbufferShader.m_Program);
                    drawable.m_Material.CreateBuffer(vk, 0, 0);
                    drawables.push_back(std::move(drawable));
                }
            });

    FlyCamController controller(init.m_Input, init.m_Window);

    while (init.m_Running) {
        if (init.PreFrame()) {
            auto viewport              = viewports::GetFullscreenViewport(init.m_Window.m_Window);
            g_Cam.m_ViewportDimensions = viewport.m_Size;
            controller.Update(g_Cam, CAST(init.m_DeltaTime, f32));
            g_MVP = GetMVP(g_Transform, g_Cam);

            for (auto& drawable: drawables) {
                if (drawable.m_Texture == nullptr
                    && drawable.m_TextureAsset != AssetHandle::BAD) {
                    if (const auto asset = init.m_AssetManager.GetAsset(drawable.m_TextureAsset)) {
                        drawable.m_Texture = &dynamic_cast<TextureAsset*>(asset)->m_Data;
                    }
                }

                drawable.m_Material.SetBuffer(vk.m_CurrentFrameIndex, 0, 0, g_MVP);
                if (drawable.m_Texture) {
                    drawable.m_Material.SetSampler(vk, "diffuse", drawable.m_Texture->m_Texture);
                }
            }

            vku::commands::RecordGraphicsCommands(vk, [&](VkCommandBuffer& cmd, uint32_t frame) {
                // G-Buffer pass (multiple render targets).
                vkCmdBeginRenderPass(
                        cmd, &fb.m_RenderPassInfo.m_RenderPassInfos[frame], VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_Pipeline);

                for (auto& drawable: drawables) {
                    vkCmdBindDescriptorSets(cmd,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            pipeline.m_PipelineLayout,
                                            0,
                                            1,
                                            &drawable.m_Material.m_DescriptorSets[0].m_Sets[frame],
                                            0,
                                            nullptr);
                    meshes::DrawMesh(viewport, drawable.m_Mesh, cmd);
                }
                vkCmdEndRenderPass(cmd);

                // Present pass with the NanoVG background.
                vku::render_passes::BeginSwapchainRenderPass(vk, cmd);

                auto        nvg    = init.m_GPU.NanoVG();
                const f32   width  = CAST(init.m_Window.m_Width, f32);
                const f32   height = CAST(init.m_Window.m_Height, f32);
                NVGpaint    bgPaint = nvgRadialGradient(nvg,
                                                     width * 0.5f,
                                                     height * 0.5f,
                                                     width * 0.2f,
                                                     width * 0.7f,
                                                     nvgRGBA(25, 30, 44, 255),
                                                     nvgRGBA(10, 12, 18, 255));
                nvgBeginPath(nvg);
                nvgRect(nvg, 0, 0, width / 2.0f, height / 2.0f);
                nvgFillPaint(nvg, bgPaint);
                nvgFill(nvg);
                nvgEndFrame(nvg);

                vkCmdEndRenderPass(cmd);
            });
        }

        profiler::ProfilerImGuiWindow(init);

        if (ImGui::Begin("Example (Deferred)")) {
            ImGuiEx::TransformEdit(g_Transform);
            ImGuiEx::CameraEdit(g_Cam);
            ImGuiEx::FlyCamControllerEdit(controller);

            auto extent = ImGui::GetContentRegionAvail();
            if (extent.x > 0.0f && extent.y > 0.0f) {
                auto& image = fb.m_ColourAttachments[0]
                                      .m_AttachmentSwapchainImages[vk.m_CurrentFrameIndex];
                ImGuiX::Image(image, extent);
            }
        }
        ImGui::End();

        init.PostFrame();
    }

    init.Quit();
}
