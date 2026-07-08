#pragma once
#include "Render/Vertex.hpp"
#include "Render/Viewport.hpp"
#include "slang-rhi.h"

namespace axm {

    struct CPUMesh
    {
        String m_Name;
        void* m_CPUMemory;
        const vertex::Layout& m_Layout;

        CPUMesh(String name, void* data, const vertex::Layout& layout) :
            m_Name(std::move(name)), m_CPUMemory(data), m_Layout(layout) { };
    };

    struct Mesh
    {
        rhi::ComPtr<rhi::IBuffer> m_VertexBuffer;
        rhi::ComPtr<rhi::IBuffer> m_IndexBuffer;
        const vertex::Layout& m_InputLayout;

        u64 m_IndexCount;
    };

    namespace meshes {
        Mesh CreateMeshFromData(rhi::IDevice* device,
                                const void* vertexData,
                                u64 vertexDataSize,
                                const u32* indexData,
                                u64 numIndices,
                                const vertex::Layout& inputLayout,
                                const char* label = "AnonMesh");


        void DrawMesh(const Viewport& viewPort, const Mesh& mesh, rhi::IRenderPassEncoder* renderPassEncoder);
    } // namespace meshes
}
