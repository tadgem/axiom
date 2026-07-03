#pragma once
#include "Viewport.hpp"
#include "slang-rhi.h"

namespace axm {
    struct Mesh {
        rhi::ComPtr<rhi::IBuffer> m_VertexBuffer;
        rhi::ComPtr<rhi::IBuffer> m_IndexBuffer;
        rhi::ComPtr<rhi::IInputLayout> m_InputLayout;

        u64 m_IndexCount;
    };

    namespace meshes {
        Mesh CreateMeshFromData(rhi::IDevice *device, const void *vertexData, u64 vertexDataSize, const u32 *indexData,
                                u64 numIndices, rhi::ComPtr<rhi::IInputLayout> inputLayout,
                                const char *label = "AnonMesh");


        void DrawMesh(const Viewport &viewPort, const Mesh &mesh, rhi::IRenderPassEncoder *renderPassEncoder);
    } // namespace meshes
}