#include "Mesh.hpp"
#include "Buffer.hpp"
#include "Vertex.hpp"

axm::Mesh axm::meshes::CreateMeshFromData(
    rhi::IDevice *device,
    const void *vertexData,
    u64 vertexDataSize,
    const u32 *indexData,
    u64 numIndices,
    rhi::ComPtr<rhi::IInputLayout> layout,
    const char *label) {

    auto vertexBuffer = buffer::CreateVertexBuffer(
        device, vertexDataSize, vertexData);

    auto indexBuffer = buffer::CreateIndexBuffer(
        device, numIndices, indexData);

    return {
        .m_VertexBuffer = vertexBuffer,
        .m_IndexBuffer = indexBuffer,
        .m_InputLayout = layout,
        .m_IndexCount =  numIndices
    };


}
