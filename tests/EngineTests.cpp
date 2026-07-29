#include "AxiomTestFramework.hpp"
#include "Render/Camera.hpp"
#include "Render/NanoVGUtils.hpp"

using namespace axm;

namespace {
    TestResult Engine_CanResizeSwapchain(AxiomEngine* e) {
        // Set window size programmatically
        SDL_SetWindowSize(e->m_Window.m_Window, 800, 600);

        // Trigger OnWindowResized directly
        SDL_Event event;
        event.type = SDL_EVENT_WINDOW_RESIZED;
        e->OnWindowResized(event);

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
        e->OnWindowResized(event);

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

    TestResult Transform_DirectionVectors(AxiomEngine* e) {
        Transform t;
        t.m_Euler = aml::Vec3(0.0f, 0.0f, 0.0f);
        t.UpdateDirectionVectors();

        AXM_TEST_ASSERT(std::abs(t.m_Forward.GetX() - 0.0f) < 0.001f, "Default Forward X should be 0");
        AXM_TEST_ASSERT(std::abs(t.m_Forward.GetY() - 0.0f) < 0.001f, "Default Forward Y should be 0");
        AXM_TEST_ASSERT(std::abs(t.m_Forward.GetZ() - (-1.0f)) < 0.001f, "Default Forward Z should be -1");

        AXM_TEST_ASSERT(std::abs(t.m_Right.GetX() - 1.0f) < 0.001f, "Default Right X should be 1");
        AXM_TEST_ASSERT(std::abs(t.m_Right.GetY() - 0.0f) < 0.001f, "Default Right Y should be 0");
        AXM_TEST_ASSERT(std::abs(t.m_Right.GetZ() - 0.0f) < 0.001f, "Default Right Z should be 0");

        AXM_TEST_ASSERT(std::abs(t.m_Up.GetX() - 0.0f) < 0.001f, "Default Up X should be 0");
        AXM_TEST_ASSERT(std::abs(t.m_Up.GetY() - 1.0f) < 0.001f, "Default Up Y should be 1");
        AXM_TEST_ASSERT(std::abs(t.m_Up.GetZ() - 0.0f) < 0.001f, "Default Up Z should be 0");

        return TestResult::Pass();
    }

    TestResult Camera_ViewMatrix(AxiomEngine* e) {
        Camera cam;
        cam.m_Transform.m_Position = aml::Vec3(0.0f, 0.0f, 5.0f);
        cam.m_Transform.m_Euler    = aml::Vec3(0.0f, 0.0f, 0.0f);

        aml::Mat44 view = Utils::CreateViewMatrix(cam.m_Transform.m_Position, cam.m_Transform.m_Euler);
        aml::Vec4  pCam = view * aml::Vec4(0.0f, 0.0f, 0.0f, 1.0f);

        AXM_TEST_ASSERT(std::abs(pCam.GetX() - 0.0f) < 0.001f, "View space X of origin should be 0");
        AXM_TEST_ASSERT(std::abs(pCam.GetY() - 0.0f) < 0.001f, "View space Y of origin should be 0");
        AXM_TEST_ASSERT(std::abs(pCam.GetZ() - (-5.0f)) < 0.001f, "View space Z of origin should be -5");

        return TestResult::Pass();
    }
}

AXM_BEGIN_TESTS("Engine Tests")

AXM_ADD_TEST(Engine_CanResizeSwapchain)
AXM_ADD_TEST(Engine_NanoVgIntegrationDoesntBreak)
AXM_ADD_TEST(Transform_DirectionVectors)
AXM_ADD_TEST(Camera_ViewMatrix)

AXM_END_TESTS()
