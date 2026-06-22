#include "../include/Utils.hpp"
rhi::ITexture *axm::Utils::CreateDepthTexture(rhi::IDevice *device, u32 w, u32 h, rhi::Format format) {
    using namespace rhi;
    TextureDesc depthDesc = {};
    depthDesc.type = TextureType::Texture2D;
    depthDesc.size.width = w;
    depthDesc.size.height = h;
    depthDesc.size.depth = 1;
    depthDesc.arrayLength = 1;
    depthDesc.mipCount = 1;
    depthDesc.format = Format::D32Float;
    depthDesc.usage = TextureUsage::DepthStencil;
    depthDesc.defaultState = ResourceState::DepthWrite;
    depthDesc.label = "Depth Texture";
    ITexture* tex;
    device->createTexture(depthDesc, nullptr, &tex);
    return tex;
}
