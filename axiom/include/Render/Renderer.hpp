#pragma once
#include "Render/GPU.hpp"
#include "Render/Shader.hpp"

namespace axm
{
class Renderer
{
private:
	DynArray<vku::Material>       p_Materials;
	DynArray<vku::VkPipelineData> p_Pipelines;

public:
	Unique<GPU> m_GPU;

	Renderer();

	vku::VkPipelineData& CreateRasterPipeline(Shader&                 shader,
	                                          vku::VertexDescription  vert,
	                                          vku::RasterizationState rasterState,
	                                          DynArray<VkFormat>      attachmentFormats);

	vku::Material&       CreateMaterial(Shader& shader);
};
}  // namespace axm
