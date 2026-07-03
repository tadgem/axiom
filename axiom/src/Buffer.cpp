#include "../include/Buffer.hpp"
#include "../include/Debug.hpp"
rhi::ComPtr<rhi::IBuffer> axm::buffer::CreateVertexBuffer(rhi::IDevice *device, u64 size, const void *data,
                                                          const char *label) {
    using namespace rhi;

    BufferDesc desc = {};
    desc.size = size;
    desc.usage = BufferUsage::VertexBuffer;
    desc.defaultState = ResourceState::VertexBuffer;
    desc.label = label;

    ComPtr<IBuffer> buffer;
    if (SLANG_FAILED(device->createBuffer(desc, data, buffer.writeRef()))) {
        AXM_LOG("Failed to create vertex buffer");
        return nullptr;
    }

    return buffer;
}
rhi::ComPtr<rhi::IBuffer> axm::buffer::CreateIndexBuffer(rhi::IDevice *device, u64 size, const void *data,
                                                         const char *label) {
    using namespace rhi;

    BufferDesc desc = {};
    desc.size = size;
    desc.usage = BufferUsage::IndexBuffer;
    desc.defaultState = ResourceState::IndexBuffer;
    desc.label = label;

    ComPtr<IBuffer> buffer;
    if (SLANG_FAILED(device->createBuffer(desc, data, buffer.writeRef()))) {
        AXM_LOG("Failed to create vertex buffer");
        return nullptr;
    }

    return buffer;
}
