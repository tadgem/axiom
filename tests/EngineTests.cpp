#include "AxiomTestFramework.hpp"
#include "Render/Camera.hpp"
#include "Render/Shader.hpp"
#include "nanovg.h"

using namespace axm;

namespace {
    TestResult Engine_Initialises(AxiomEngine* e) {
        AXM_TEST_ASSERT(e->m_OK, "AxiomEngine::Init reported failure");
        AXM_TEST_ASSERT(e->m_VK != nullptr, "vku state was not created");
        AXM_TEST_ASSERT(e->m_Window.m_Window != nullptr, "SDL window was not created");
        AXM_TEST_ASSERT(e->m_VK->m_SwapChainImageExtent.width > 0, "Swapchain width invalid");
        AXM_TEST_ASSERT(e->m_VK->m_SwapChainImageExtent.height > 0, "Swapchain height invalid");
        return TestResult::Pass();
    }

    TestResult Engine_CanResizeWindow(AxiomEngine* e) {
        SDL_SetWindowSize(e->m_Window.m_Window, 800, 600);

        SDL_Event event;
        event.type = SDL_EVENT_WINDOW_RESIZED;
        e->OnWindowResized(event);

        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(e->m_Window.m_Window, &w, &h);

        AXM_TEST_ASSERT(e->m_Window.m_Width == static_cast<u32>(w), "Window width mismatch");
        AXM_TEST_ASSERT(e->m_Window.m_Height == static_cast<u32>(h), "Window height mismatch");

        return TestResult::Pass();
    }

    TestResult Engine_NanoVgContextWorks(AxiomEngine* e) {
        NVGcontext* nvg = e->m_GPU.NanoVG();
        AXM_TEST_ASSERT(nvg != nullptr, "NanoVG context was not created by vku");

        nvgBeginFrame(nvg, 512, 512, 1.0f);
        nvgBeginPath(nvg);
        nvgRect(nvg, 10, 10, 100, 100);
        nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
        nvgFill(nvg);

        return TestResult::Pass();
    }

    TestResult Transform_DirectionVectors(AxiomEngine* e) {
        (void)e;
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
        (void)e;
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

    TestResult Shaders_CompileFromSource(AxiomEngine* e) {
        Shader shader(*e->m_VK, "resources/shaders/cube.vert", "resources/shaders/cube.frag");
        AXM_TEST_ASSERT(shader.Valid(), "Expected cube shader to compile from GLSL source");
        return TestResult::Pass();
    }

    // Vulkan expects Y-down NDC and a depth range of [0, 1]. The camera must
    // therefore flip Y and remap z from the OpenGL [-1, 1] convention.
    TestResult Camera_VulkanProjectionConventions(AxiomEngine* e) {
        (void)e;
        Camera cam;
        cam.m_FOV = 90.0f;
        cam.m_NearPlane = 0.1f;
        cam.m_FarPlane = 100.0f;
        cam.m_ViewportDimensions = aml::Float2(1.0f, 1.0f);

        const aml::Mat44 proj = cam.GetProjectionMatrix();

        // A point on the near plane (camera looking down -Z) has clip depth 0,
        // and a point on the far plane has clip depth 1 (after perspective
        // divide, w == -z_view for a right-handed projection).
        const float nearZ = -cam.m_NearPlane;
        const float farZ = -cam.m_FarPlane;

        aml::Vec4 nearClip = proj * aml::Vec4(0.0f, 0.0f, nearZ, 1.0f);
        aml::Vec4 farClip = proj * aml::Vec4(0.0f, 0.0f, farZ, 1.0f);
        const float nearDepth = nearClip.GetZ() / nearClip.GetW();
        const float farDepth = farClip.GetZ() / farClip.GetW();

        AXM_TEST_ASSERT(std::abs(nearDepth - 0.0f) < 0.001f, "Near plane should map to depth 0");
        AXM_TEST_ASSERT(std::abs(farDepth - 1.0f) < 0.001f, "Far plane should map to depth 1");

        // A point above the view centre must map to negative Y in Vulkan's
        // Y-down clip space (i.e. the projection flips Y).
        aml::Vec4 upClip = proj * aml::Vec4(0.0f, 1.0f, nearZ, 1.0f);
        AXM_TEST_ASSERT(upClip.GetY() / upClip.GetW() < 0.0f, "Projection should flip Y for Vulkan");

        return TestResult::Pass();
    }
}

AXM_BEGIN_TESTS("Engine Tests")

AXM_ADD_TEST(Engine_Initialises)
AXM_ADD_TEST(Engine_CanResizeWindow)
AXM_ADD_TEST(Engine_NanoVgContextWorks)
AXM_ADD_TEST(Transform_DirectionVectors)
AXM_ADD_TEST(Camera_ViewMatrix)
AXM_ADD_TEST(Camera_VulkanProjectionConventions)
AXM_ADD_TEST(Shaders_CompileFromSource)

AXM_END_TESTS()
