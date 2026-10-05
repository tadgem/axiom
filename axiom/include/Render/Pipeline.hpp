#pragma once
#include "Render/Shader.hpp"
#include "Render/Vertex.hpp"
#include "vku/vku.h"

namespace axm::pipeline {

    vku::VkPipelineData CreateRasterPipeline(vku::VkState&          vk,
                                             Shader&                shader,
                                             vku::VertexDescription& vertDesc,
                                             vku::RasterizationState& raster,
                                             VkRenderPass           renderPass,
                                             VkExtent2D             extent,
                                             u32                    colourCount = 1);

    vku::VkPipelineData CreateComputePipeline(vku::VkState& vk, Shader& shader);

} // namespace axm::pipeline
