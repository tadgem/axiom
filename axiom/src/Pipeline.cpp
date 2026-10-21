#include "Render/Pipeline.hpp"

#include "Core/Debug.hpp"

vku::VkPipelineData axm::pipeline::CreateRasterPipeline(vku::VkState&            vk,
                                                        Shader&                  shader,
                                                        vku::VertexDescription&  vertDesc,
                                                        vku::RasterizationState& raster,
                                                        VkRenderPass             renderPass,
                                                        VkExtent2D               extent,
                                                        u32                      colourCount)
{
	if (!shader.Valid())
	{
		AXM_LOG_ERROR("Cannot create raster pipeline from an invalid shader");
		return {};
	}

	raster.m_FrontFace = VK_FRONT_FACE_CLOCKWISE;

	return vku::pipelines::CreateRasterPipeline(
	        vk, shader.m_Program, vertDesc, raster, renderPass, extent, colourCount);
}

vku::VkPipelineData axm::pipeline::CreateRasterDynamicPipeline(vku::VkState&                 vk,
                                                               Shader&                       shader,
                                                               vku::VertexDescription&       vertDesc,
                                                               vku::RasterizationState&      raster,
                                                               VkExtent2D                    extent,
                                                               DynArray<VkFormat>            formats,
                                                               vku::PipelineAttachmentState* attachmentState)
{
	if (!shader.Valid())
	{
		AXM_LOG_ERROR("Cannot create raster pipeline from an invalid shader");
		return {};
	}

	raster.m_FrontFace = VK_FRONT_FACE_CLOCKWISE;

	if (attachmentState)
	{
		return vku::pipelines::CreateDynamicRasterPipeline(
		        vk, shader.m_Program, vertDesc, raster, extent, formats.data(), formats.size(), *attachmentState);
	}
	else
	{
		return vku::pipelines::CreateDynamicRasterPipeline(
		        vk, shader.m_Program, vertDesc, raster, extent, formats.data(), formats.size());
	}
}

vku::VkPipelineData axm::pipeline::CreateComputePipeline(vku::VkState& vk, Shader& shader)
{
	if (!shader.Valid() || shader.m_Program.m_Stages.empty())
	{
		AXM_LOG_ERROR("Cannot create compute pipeline from an invalid shader");
		return {};
	}

	return vku::pipelines::CreateComputePipeline(
	        vk, shader.m_Program.m_Stages[0].m_StageBinary, shader.m_Program.m_DescriptorSetLayout);
}
