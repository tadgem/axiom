#include "Buffer.hpp"
#include "Log.hpp"
rhi::IBuffer *axm::buffer::CreateVertexBuffer(rhi::IDevice *device, u64 size, const void* data, const char *label) {
    using namespace rhi;

    BufferDesc desc = {};
    desc.size = size;
    desc.usage = BufferUsage::VertexBuffer;
    desc.defaultState = ResourceState::VertexBuffer;
    desc.label = label;

    IBuffer* buffer;
    if (SLANG_FAILED(device->createBuffer(desc, data, &buffer))) {
        AXM_LOG("Failed to create vertex buffer");
        return nullptr;
    }

    return buffer;

}
rhi::IBuffer *axm::buffer::CreateIndexBuffer(rhi::IDevice *device, u64 size, const void *data, const char *label) {
    using namespace rhi;

    BufferDesc desc = {};
    desc.size = size;
    desc.usage = BufferUsage::IndexBuffer;
    desc.defaultState = ResourceState::IndexBuffer;
    desc.label = label;

    IBuffer* buffer;
    if (SLANG_FAILED(device->createBuffer(desc, data, &buffer))) {
        AXM_LOG("Failed to create vertex buffer");
        return nullptr;
    }

    return buffer;
}
