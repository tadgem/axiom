#include "Render/Mesh.hpp"
#include "Core/Debug.hpp"

#include <cstring>

axm::Mesh axm::meshes::CreateMeshFromData(vku::VkState& vk,
                                          const void*    vertexData,
                                          u64            vertexDataSize,
                                          const u32*     indexData,
                                          u64            numIndices,
                                          const char*    label) {
    (void)label;
    Mesh mesh{};

    vku::Buffer vertexBuffer = vku::buffers::CreateBuffer(
            vk,
            vertexDataSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vku::MappedBuffer staging = vku::buffers::CreateMappedBuffer(
            vk,
            vertexDataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::memcpy(staging.m_MappedAddr, vertexData, static_cast<size_t>(vertexDataSize));
    vku::buffers::CopyBuffer(vk, staging.m_GpuBuffer, vertexBuffer.m_GpuBuffer, vertexDataSize);
    staging.Free(vk);

    vku::Buffer indexBuffer =
            vku::buffers::CreateIndexBuffer(vk, const_cast<u32*>(indexData), static_cast<size_t>(numIndices));

    mesh.m_VertexBuffer = vertexBuffer;
    mesh.m_IndexBuffer  = indexBuffer;
    mesh.m_IndexCount   = numIndices;
    mesh.m_Valid        = true;

    return mesh;
}

void axm::meshes::Bind(const Mesh& mesh, VkCommandBuffer cmd) {
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.m_VertexBuffer.m_GpuBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.m_IndexBuffer.m_GpuBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void axm::meshes::DrawMesh(const Viewport& viewPort, const Mesh& mesh, VkCommandBuffer cmd) {
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = viewPort.m_Size.x;
    viewport.height   = viewPort.m_Size.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { CAST(viewPort.m_Size.x, u32), CAST(viewPort.m_Size.y, u32) };

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    Bind(mesh, cmd);
    vkCmdDrawIndexed(cmd, CAST(mesh.m_IndexCount, u32), 1, 0, 0, 0);
}
