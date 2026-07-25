#include "AxiomTestFramework.hpp"
#include "Render/NanoVGUtils.hpp"

using namespace axm;

namespace {
    TestResult Engine_CanResizeSwapchain(AxiomEngine* e) {
        // Set window size programmatically
        SDL_SetWindowSize(e->m_Window.m_Window, 800, 600);

        // Trigger OnWindowResized directly
        SDL_Event event;
        event.type = SDL_EVENT_WINDOW_RESIZED;
        engine::OnWindowResized(*e, event);

        // Get actual sizes in pixels
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(e->m_Window.m_Window, &w, &h);

        AXM_TEST_ASSERT(e->m_Window.m_Width == static_cast<u32>(w), "Window width mismatch after resizing to 800x600");
        AXM_TEST_ASSERT(e->m_Window.m_Height == static_cast<u32>(h),
                        "Window height mismatch after resizing to 800x600");
        AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.width == static_cast<uint32_t>(w),
                        "Depth texture width mismatch");
        AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.height == static_cast<uint32_t>(h),
                        "Depth texture height mismatch");

        // Restore window size to 1024x768
        SDL_SetWindowSize(e->m_Window.m_Window, 1024, 768);
        engine::OnWindowResized(*e, event);

        SDL_GetWindowSizeInPixels(e->m_Window.m_Window, &w, &h);
        AXM_TEST_ASSERT(e->m_Window.m_Width == static_cast<u32>(w),
                        "Window width mismatch after restoring to 1024x768");
        AXM_TEST_ASSERT(e->m_Window.m_Height == static_cast<u32>(h),
                        "Window height mismatch after restoring to 1024x768");
        AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.width == static_cast<uint32_t>(w),
                        "Depth texture width mismatch after restore");
        AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.height == static_cast<uint32_t>(h),
                        "Depth texture height mismatch after restore");

        return TestResult::Pass();
    }

    TestResult Engine_NanoVgIntegrationDoesntBreak(AxiomEngine* e) {
        NVGcontext* nvg = axm::nanovg::CreateContext(e->m_GPU.m_Device);
        AXM_TEST_ASSERT(nvg != nullptr, "Failed to create NanoVG Slang-RHI context");

        bool pipelineCreated = axm::nanovg::CreatePipeline(nvg, rhi::Format::RGBA8Unorm, rhi::Format::D32Float);
        AXM_TEST_ASSERT(pipelineCreated, "Failed to create NanoVG render pipeline");

        nvgBeginFrame(nvg, 512, 512, 1.0f);
        nvgBeginPath(nvg);
        nvgRect(nvg, 10, 10, 100, 100);
        nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
        nvgFill(nvg);
        nvgEndFrame(nvg);

        rhi::ComPtr<rhi::ITexture> testTex;
        rhi::TextureDesc           desc = { };
        desc.type                       = rhi::TextureType::Texture2D;
        desc.size                       = { 128, 128, 1 };
        desc.arrayLength                = 1;
        desc.mipCount                   = 1;
        desc.format                     = rhi::Format::RGBA8Unorm;
        desc.usage                      = rhi::TextureUsage::ShaderResource;
        desc.defaultState               = rhi::ResourceState::ShaderResource;
        e->m_GPU.m_Device->createTexture(desc, nullptr, testTex.writeRef());

        if (testTex) {
            auto view = testTex->getDefaultView();
            int  img  = axm::nanovg::CreateImageFromTextureView(nvg, view.get());
            AXM_TEST_ASSERT(img > 0, "Failed to register texture view with NanoVG");
            axm::nanovg::DeleteImage(nvg, img);
        }

        axm::nanovg::DestroyContext(nvg);
        return TestResult::Pass();
    }
}

AXM_BEGIN_TESTS("Engine Tests")

AXM_ADD_TEST(Engine_CanResizeSwapchain)
AXM_ADD_TEST(Engine_NanoVgIntegrationDoesntBreak)

AXM_END_TESTS()
