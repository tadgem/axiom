#include "AxiomTestFramework.hpp"

using namespace axm;

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
    AXM_TEST_ASSERT(e->m_Window.m_Height == static_cast<u32>(h), "Window height mismatch after resizing to 800x600");
    AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.width == static_cast<uint32_t>(w),
                    "Depth texture width mismatch");
    AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.height == static_cast<uint32_t>(h),
                    "Depth texture height mismatch");

    // Restore window size to 1024x768
    SDL_SetWindowSize(e->m_Window.m_Window, 1024, 768);
    engine::OnWindowResized(*e, event);

    SDL_GetWindowSizeInPixels(e->m_Window.m_Window, &w, &h);
    AXM_TEST_ASSERT(e->m_Window.m_Width == static_cast<u32>(w), "Window width mismatch after restoring to 1024x768");
    AXM_TEST_ASSERT(e->m_Window.m_Height == static_cast<u32>(h), "Window height mismatch after restoring to 1024x768");
    AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.width == static_cast<uint32_t>(w),
                    "Depth texture width mismatch after restore");
    AXM_TEST_ASSERT(e->m_GPU.m_SwapchainDepthImage->getDesc().size.height == static_cast<uint32_t>(h),
                    "Depth texture height mismatch after restore");

    return TestResult::Pass();
}

AXM_BEGIN_TESTS("Engine Tests")

AXM_ADD_TEST(Engine_CanResizeSwapchain)

AXM_END_TESTS()
