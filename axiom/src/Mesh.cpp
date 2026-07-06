#include "../include/Render/Mesh.hpp"
#include "../include/Render/Buffer.hpp"
#include "../include/Render/Vertex.hpp"

axm::Mesh axm::meshes::CreateMeshFromData(rhi::IDevice *device, const void *vertexData, u64 vertexDataSize,
                                          const u32 *indexData, u64 numIndices, rhi::ComPtr<rhi::IInputLayout> layout,
                                          const char *label) {

    auto vertexBuffer = buffer::CreateVertexBuffer(device, vertexDataSize, vertexData);

    auto indexBuffer = buffer::CreateIndexBuffer(device, numIndices, indexData);

    return {.m_VertexBuffer = vertexBuffer,
            .m_IndexBuffer = indexBuffer,
            .m_InputLayout = layout,
            .m_IndexCount = numIndices};
}


void axm::meshes::DrawMesh(const Viewport &viewPort, const Mesh &mesh, rhi::IRenderPassEncoder *renderPassEncoder) {

    rhi::RenderState renderState = {};
    renderState.viewports[0] = rhi::Viewport::fromSize(viewPort.m_Size.x, viewPort.m_Size.y);

    renderState.viewportCount = 1;
    renderState.scissorRects[0] = rhi::ScissorRect::fromSize(viewPort.m_Size.x, viewPort.m_Size.y);

    renderState.scissorRectCount = 1;
    renderState.vertexBuffers[0].buffer = mesh.m_VertexBuffer;
    renderState.vertexBufferCount = 1;
    renderState.indexBuffer.buffer = mesh.m_IndexBuffer;
    renderState.indexFormat = rhi::IndexFormat::Uint32;
    renderPassEncoder->setRenderState(renderState);

    rhi::DrawArguments drawArgs = {};
    drawArgs.vertexCount = mesh.m_IndexCount;
    renderPassEncoder->drawIndexed(drawArgs);
}
