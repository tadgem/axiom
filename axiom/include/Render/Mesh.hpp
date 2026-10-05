#pragma once
#include "Core/STL.hpp"
#include "Render/Viewport.hpp"
#include "vku/vku.h"

namespace axm {

    struct CPUMesh
    {
        String   m_Name;
        void*    m_CPUMemory;
        CPUMesh(String name, void* data) : m_Name(std::move(name)), m_CPUMemory(data) { };
    };

    struct Mesh
    {
        vku::Buffer m_VertexBuffer;
        vku::Buffer m_IndexBuffer;
        u64         m_IndexCount = 0;
        bool        m_Valid      = false;

        void        Free(vku::VkState& vk) {
            m_VertexBuffer.Free(vk);
            m_IndexBuffer.Free(vk);
            m_Valid = false;
        }
    };

    namespace meshes {
        Mesh CreateMeshFromData(vku::VkState& vk,
                                const void*    vertexData,
                                u64            vertexDataSize,
                                const u32*     indexData,
                                u64            numIndices,
                                const char*    label = "AnonMesh");

        void Bind(const Mesh& mesh, VkCommandBuffer cmd);
        void DrawMesh(const Viewport& viewPort, const Mesh& mesh, VkCommandBuffer cmd);
    } // namespace meshes
} // namespace axm
