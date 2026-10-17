#pragma once

#include "vku/vku.h"

namespace axm
{
// Engine-owned shared image samplers. The vku::VkState itself is the
// single source of truth for device, queues and swapchain.
struct Samplers
{
	VkSampler m_LinearClampSampler  = VK_NULL_HANDLE;
	VkSampler m_LinearWrapSampler   = VK_NULL_HANDLE;
	VkSampler m_NearestClampSampler = VK_NULL_HANDLE;
	VkSampler m_NearestWrapSampler  = VK_NULL_HANDLE;

	void      Free(vku::VkState& vk)
	{
		vkDestroySampler(vk.m_LogicalDevice, m_LinearClampSampler, nullptr);
		vkDestroySampler(vk.m_LogicalDevice, m_LinearWrapSampler, nullptr);
		vkDestroySampler(vk.m_LogicalDevice, m_NearestClampSampler, nullptr);
		vkDestroySampler(vk.m_LogicalDevice, m_NearestWrapSampler, nullptr);
	}
};
}  // namespace axm
