#pragma once

#include "vku/vku.h"

struct NVGcontext;

namespace axm {
    // Thin convenience view over the vku::VkState owned by the engine. The
    // vku state is the single source of truth for device, queues and swapchain.
    struct GPU
    {
        vku::VkState* m_VK = nullptr;

        VkSampler     m_LinearClampSampler = VK_NULL_HANDLE;
        VkSampler     m_LinearWrapSampler  = VK_NULL_HANDLE;

        NO_DISCARD vku::VkState&       State() const { return *m_VK; }
        // NanoVG context is recreated on swapchain recreation, so always
        // resolve it from the live vku state.
        NO_DISCARD NVGcontext*         NanoVG() const { return m_VK->m_NanoVG; }
        NO_DISCARD VkDevice            Device() const { return m_VK->m_LogicalDevice; }
        NO_DISCARD VkExtent2D          SwapchainExtent() const { return m_VK->m_SwapChainImageExtent; }
        NO_DISCARD VkFormat            SwapchainFormat() const { return m_VK->m_SwapChainImageFormat; }
    };
}
