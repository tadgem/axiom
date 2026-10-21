#include "Render/Renderer.hpp"

#include "VkSDL.h"

vku::VkPipelineData& axm::Renderer::CreateRasterPipeline(Shader&                 shader,
                                                         vku::VertexDescription  vert,
                                                         vku::RasterizationState rasterState,
                                                         DynArray<VkFormat>      attachmentFormats)
{
	p_Pipelines.push_back(vku::pipelines::CreateDynamicRasterPipeline(m_GPU->m_VK,
	                                                                  shader.m_Program,
	                                                                  vert,
	                                                                  rasterState,
	                                                                  VkExtent2D{1920, 1080},
	                                                                  attachmentFormats.data(),
	                                                                  attachmentFormats.size()));
	return p_Pipelines.back();
}

vku::Material& axm::Renderer::CreateMaterial(Shader& shader)
{
	p_Materials.push_back(vku::Material::Create(m_GPU->m_VK, shader.m_Program));
	return p_Materials.back();
}

axm::Renderer::Renderer()
{
	vku::VkState vk = vku::init::Create<vku::VkSDL>("AXIOM", 1920, 1080, false, true);
	m_GPU           = MakeUnique<GPU>(vk);
}
