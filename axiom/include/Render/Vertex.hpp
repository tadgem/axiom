#pragma once
#include <cstddef>
#include "Core/Maths.hpp"
#include "Core/Prim.hpp"
#include "vku/vku.h"

namespace axm::vertex {

    // Interleaved position(3) / normal(3) / uv(2) vertex used by models.
    struct PosNormalUV
    {
        aml::Float3 m_Pos;
        aml::Float3 m_Normal;
        aml::Float2 m_UV;

        static vku::VertexDescription GetVertexDescription(vku::VkState& vk) {
            vku::VertexDescription desc(*vk.m_CPUAllocator);

            VkVertexInputBindingDescription binding{};
            binding.binding   = 0;
            binding.stride    = sizeof(PosNormalUV);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            desc.m_BindingDescriptions.push_back(binding);

            VkVertexInputAttributeDescription pos{};
            pos.binding  = 0;
            pos.location = 0;
            pos.format   = VK_FORMAT_R32G32B32_SFLOAT;
            pos.offset   = static_cast<uint32_t>(offsetof(PosNormalUV, m_Pos));
            desc.m_AttributeDescriptions.push_back(pos);

            VkVertexInputAttributeDescription normal{};
            normal.binding  = 0;
            normal.location = 1;
            normal.format   = VK_FORMAT_R32G32B32_SFLOAT;
            normal.offset   = static_cast<uint32_t>(offsetof(PosNormalUV, m_Normal));
            desc.m_AttributeDescriptions.push_back(normal);

            VkVertexInputAttributeDescription uv{};
            uv.binding  = 0;
            uv.location = 2;
            uv.format   = VK_FORMAT_R32G32_SFLOAT;
            uv.offset   = static_cast<uint32_t>(offsetof(PosNormalUV, m_UV));
            desc.m_AttributeDescriptions.push_back(uv);

            return desc;
        }
    };
} // namespace axm::vertex
