#include "imgui_impl_slang_rhi.h"
#include <slang-rhi/shader-cursor.h>
#include <iostream>
#include <vector>

namespace {
    rhi::IDevice* g_Device = nullptr;
    rhi::ComPtr<rhi::IInputLayout> g_InputLayout;
    rhi::ComPtr<rhi::IRenderPipeline> g_Pipeline;
    rhi::ComPtr<rhi::ISampler> g_FontSampler;
    rhi::ComPtr<rhi::ITexture> g_FontTexture;
    rhi::ComPtr<rhi::ITextureView> g_FontTextureView;
    
    static const int kMaxFramesInFlight = 3;
    rhi::ComPtr<rhi::IBuffer> g_VertexBuffers[kMaxFramesInFlight];
    rhi::ComPtr<rhi::IBuffer> g_IndexBuffers[kMaxFramesInFlight];
    uint64_t g_VertexBufferSize[kMaxFramesInFlight] = {0};
    uint64_t g_IndexBufferSize[kMaxFramesInFlight] = {0};
    int g_FrameIndex = 0;

    bool g_IsSrgbTarget = false;
}

const std::string imgui_slang_shader_src =
"struct VertexInput"
"{"
    "float2 pos : POSITION;"
    "float2 uv  : TEXCOORD;"
    "float4 col : COLOR;"
"};"
"struct VertexStageOutput"
"{"
"    float4 sv_position : SV_Position;"
"    float2 uv : TEXCOORD;"
"    float4 col : COLOR;"
"};"
"uniform float2 uScale;"
"uniform float2 uTranslate;"
"[shader(\"vertex\")]"
"VertexStageOutput vertexMain(VertexInput input)"
"{"
    "VertexStageOutput output;"
    "output.sv_position = float4(input.pos * uScale + uTranslate, 0.0, 1.0);"
    "output.uv = input.uv;"
    "output.col = input.col;"
    "return output;"
"}"
"Texture2D fontTexture;"
"SamplerState fontSampler;"
"uniform bool isSrgbTarget;"
"[shader(\"fragment\")]"
"float4 fragmentMain(VertexStageOutput input) : SV_Target"
"{"
"    float4 col = input.col * fontTexture.Sample(fontSampler, input.uv);"
"    if (isSrgbTarget)"
"    {"
"        col.rgb = pow(col.rgb, 2.2);"
"    }"
"    return col;"
"}";

bool ImGui_ImplSlangRHI_Init(rhi::IDevice* device, rhi::Format renderTargetFormat) {
    g_Device = device;
    g_FrameIndex = 0;
    
    g_IsSrgbTarget = (renderTargetFormat == rhi::Format::RGBA8UnormSrgb ||
                      renderTargetFormat == rhi::Format::BGRA8UnormSrgb ||
                      renderTargetFormat == rhi::Format::BGRX8UnormSrgb);

    // Load shader module at runtime
    rhi::ComPtr<slang::IBlob> diagnostics;
    slang::IModule* module = g_Device->getSlangSession()->loadModuleFromSourceString("imgui", ".", imgui_slang_shader_src.c_str(), diagnostics.writeRef());
    // slang::IModule* module = g_Device->getSlangSession()->loadModule("imgui", diagnostics.writeRef());
    if (diagnostics) {
        std::cout << "ImGui Shader compilation messages:\n" << (const char*)diagnostics->getBufferPointer() << std::endl;
    }
    if (!module) {
        std::cerr << "ImGui backend: Failed to load imgui shader module" << std::endl;
        return false;
    }

    slang::IEntryPoint* vertexEntryPoint = nullptr;
    module->findEntryPointByName("vertexMain", &vertexEntryPoint);
    slang::IEntryPoint* fragmentEntryPoint = nullptr;
    module->findEntryPointByName("fragmentMain", &fragmentEntryPoint);

    std::vector<slang::IComponentType*> entryPoints = { vertexEntryPoint, fragmentEntryPoint };

    rhi::ShaderProgramDesc programDesc = {};
    programDesc.linkingStyle = rhi::LinkingStyle::SingleProgram;
    programDesc.slangEntryPoints = entryPoints.data();
    programDesc.slangEntryPointCount = (uint32_t)entryPoints.size();
    programDesc.slangGlobalScope = module;

    rhi::ComPtr<rhi::IShaderProgram> program;
    if (SLANG_FAILED(g_Device->createShaderProgram(programDesc, program.writeRef(), diagnostics.writeRef()))) {
        std::cerr << "ImGui backend: Failed to create shader program" << std::endl;
        if (diagnostics) {
            std::cerr << (const char*)diagnostics->getBufferPointer() << std::endl;
        }
        return false;
    }

    // Define vertex input layout
    rhi::VertexStreamDesc vertexStreams[] = {
        {sizeof(ImDrawVert), rhi::InputSlotClass::PerVertex, 0},
    };
    rhi::InputElementDesc inputElements[] = {
        {"POSITION", 0, rhi::Format::RG32Float, offsetof(ImDrawVert, pos), 0},
        {"TEXCOORD", 0, rhi::Format::RG32Float, offsetof(ImDrawVert, uv), 0},
        {"COLOR", 0, rhi::Format::RGBA8Unorm, offsetof(ImDrawVert, col), 0},
    };
    rhi::InputLayoutDesc inputLayoutDesc = {};
    inputLayoutDesc.inputElements = inputElements;
    inputLayoutDesc.inputElementCount = sizeof(inputElements) / sizeof(inputElements[0]);
    inputLayoutDesc.vertexStreams = vertexStreams;
    inputLayoutDesc.vertexStreamCount = sizeof(vertexStreams) / sizeof(vertexStreams[0]);

    if (SLANG_FAILED(g_Device->createInputLayout(inputLayoutDesc, g_InputLayout.writeRef()))) {
        std::cerr << "ImGui backend: Failed to create input layout" << std::endl;
        return false;
    }

    // Define render pipeline
    rhi::ColorTargetDesc colorTarget = {};
    colorTarget.format = renderTargetFormat;
    colorTarget.enableBlend = true;
    colorTarget.color.srcFactor = rhi::BlendFactor::SrcAlpha;
    colorTarget.color.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    colorTarget.color.op = rhi::BlendOp::Add;
    colorTarget.alpha.srcFactor = rhi::BlendFactor::One;
    colorTarget.alpha.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    colorTarget.alpha.op = rhi::BlendOp::Add;

    rhi::DepthStencilDesc depthStencilDesc = {};
    depthStencilDesc.depthTestEnable = false;
    depthStencilDesc.depthWriteEnable = false;

    rhi::RasterizerDesc rasterizerDesc = {};
    rasterizerDesc.fillMode = rhi::FillMode::Solid;
    rasterizerDesc.cullMode = rhi::CullMode::None;
    rasterizerDesc.scissorEnable = true;

    rhi::RenderPipelineDesc pipelineDesc = {};
    pipelineDesc.program = program;
    pipelineDesc.inputLayout = g_InputLayout;
    pipelineDesc.targets = &colorTarget;
    pipelineDesc.targetCount = 1;
    pipelineDesc.depthStencil = depthStencilDesc;
    pipelineDesc.rasterizer = rasterizerDesc;

    if (SLANG_FAILED(g_Device->createRenderPipeline(pipelineDesc, g_Pipeline.writeRef()))) {
        std::cerr << "ImGui backend: Failed to create render pipeline" << std::endl;
        return false;
    }

    // Configure sampler state
    rhi::SamplerDesc samplerDesc = {};
    samplerDesc.minFilter = rhi::TextureFilteringMode::Linear;
    samplerDesc.magFilter = rhi::TextureFilteringMode::Linear;
    samplerDesc.mipFilter = rhi::TextureFilteringMode::Linear;
    samplerDesc.addressU = rhi::TextureAddressingMode::ClampToEdge;
    samplerDesc.addressV = rhi::TextureAddressingMode::ClampToEdge;
    samplerDesc.addressW = rhi::TextureAddressingMode::ClampToEdge;

    if (SLANG_FAILED(g_Device->createSampler(samplerDesc, g_FontSampler.writeRef()))) {
        std::cerr << "ImGui backend: Failed to create font sampler" << std::endl;
        return false;
    }

    // Configure font atlas texture
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    rhi::TextureDesc fontDesc = {};
    fontDesc.type = rhi::TextureType::Texture2D;
    fontDesc.size.width = width;
    fontDesc.size.height = height;
    fontDesc.size.depth = 1;
    fontDesc.arrayLength = 1;
    fontDesc.mipCount = 1;
    fontDesc.format = rhi::Format::RGBA8Unorm;
    fontDesc.usage = rhi::TextureUsage::ShaderResource;
    fontDesc.defaultState = rhi::ResourceState::ShaderResource;
    fontDesc.label = "ImGui Font Texture";

    rhi::SubresourceData initData = {};
    initData.data = pixels;
    initData.rowPitch = (rhi::Size)width * 4;

    if (SLANG_FAILED(g_Device->createTexture(fontDesc, &initData, g_FontTexture.writeRef()))) {
        std::cerr << "ImGui backend: Failed to create font texture" << std::endl;
        return false;
    }

    g_FontTextureView = g_FontTexture->getDefaultView();
    if (!g_FontTextureView) {
        std::cerr << "ImGui backend: Failed to get default view for font texture" << std::endl;
        return false;
    }

    io.Fonts->SetTexID((ImTextureID)g_FontTextureView.get());

    return true;
}

void ImGui_ImplSlangRHI_Shutdown() {
    g_Pipeline.setNull();
    g_InputLayout.setNull();
    g_FontSampler.setNull();
    g_FontTextureView.setNull();
    g_FontTexture.setNull();
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        g_VertexBuffers[i].setNull();
        g_IndexBuffers[i].setNull();
        g_VertexBufferSize[i] = 0;
        g_IndexBufferSize[i] = 0;
    }
    g_FrameIndex = 0;
    g_Device = nullptr;
}

void ImGui_ImplSlangRHI_NewFrame() {
    g_FrameIndex = (g_FrameIndex + 1) % kMaxFramesInFlight;
}

void ImGui_ImplSlangRHI_RenderDrawData(ImDrawData* drawData, rhi::ICommandEncoder* commandEncoder, rhi::IRenderPassEncoder* renderPassEncoder) {
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        return;

    // Calculate total vertex/index counts
    uint32_t totalVertexCount = 0;
    uint32_t totalIndexCount = 0;
    for (int i = 0; i < drawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = drawData->CmdLists[i];
        totalVertexCount += cmdList->VtxBuffer.Size;
        totalIndexCount += cmdList->IdxBuffer.Size;
    }

    if (totalVertexCount == 0 || totalIndexCount == 0)
        return;

    uint64_t vertexSizeNeeded = totalVertexCount * sizeof(ImDrawVert);
    uint64_t indexSizeNeeded = totalIndexCount * sizeof(ImDrawIdx);

    rhi::ComPtr<rhi::IBuffer>& vertexBuffer = g_VertexBuffers[g_FrameIndex];
    rhi::ComPtr<rhi::IBuffer>& indexBuffer = g_IndexBuffers[g_FrameIndex];
    uint64_t& vertexBufferSize = g_VertexBufferSize[g_FrameIndex];
    uint64_t& indexBufferSize = g_IndexBufferSize[g_FrameIndex];

    // Recreate Vertex Buffer if needed
    if (!vertexBuffer || vertexBufferSize < vertexSizeNeeded) {
        vertexBuffer.setNull();
        vertexBufferSize = vertexSizeNeeded + 5000 * sizeof(ImDrawVert);
        rhi::BufferDesc desc = {};
        desc.size = vertexBufferSize;
        desc.usage = rhi::BufferUsage::VertexBuffer;
        desc.defaultState = rhi::ResourceState::VertexBuffer;
        desc.memoryType = rhi::MemoryType::Upload;
        desc.label = "ImGui Vertex Buffer";
        if (SLANG_FAILED(g_Device->createBuffer(desc, nullptr, vertexBuffer.writeRef()))) {
            std::cerr << "ImGui backend: Failed to allocate vertex buffer" << std::endl;
            return;
        }
    }

    // Recreate Index Buffer if needed
    if (!indexBuffer || indexBufferSize < indexSizeNeeded) {
        indexBuffer.setNull();
        indexBufferSize = indexSizeNeeded + 10000 * sizeof(ImDrawIdx);
        rhi::BufferDesc desc = {};
        desc.size = indexBufferSize;
        desc.usage = rhi::BufferUsage::IndexBuffer;
        desc.defaultState = rhi::ResourceState::IndexBuffer;
        desc.memoryType = rhi::MemoryType::Upload;
        desc.label = "ImGui Index Buffer";
        if (SLANG_FAILED(g_Device->createBuffer(desc, nullptr, indexBuffer.writeRef()))) {
            std::cerr << "ImGui backend: Failed to allocate index buffer" << std::endl;
            return;
        }
    }

    // Copy data
    void* vertexDataMap = nullptr;
    void* indexDataMap = nullptr;
    if (SLANG_SUCCEEDED(g_Device->mapBuffer(vertexBuffer, rhi::CpuAccessMode::Write, &vertexDataMap)) &&
        SLANG_SUCCEEDED(g_Device->mapBuffer(indexBuffer, rhi::CpuAccessMode::Write, &indexDataMap))) {
        
        ImDrawVert* vtxDst = (ImDrawVert*)vertexDataMap;
        ImDrawIdx* idxDst = (ImDrawIdx*)indexDataMap;
        
        for (int i = 0; i < drawData->CmdListsCount; i++) {
            const ImDrawList* cmdList = drawData->CmdLists[i];
            memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmdList->VtxBuffer.Size;
            idxDst += cmdList->IdxBuffer.Size;
        }
        
        g_Device->unmapBuffer(vertexBuffer);
        g_Device->unmapBuffer(indexBuffer);
    } else {
        std::cerr << "ImGui backend: Failed to map buffers" << std::endl;
        return;
    }

    // Prepare scale and translate vectors
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    
    float uScale[2] = { 2.0f / (R - L), 2.0f / (T - B) };
    float uTranslate[2] = { (L + R) / (L - R), (T + B) / (B - T) };

    // Render draw lists
    uint32_t globalVertexOffset = 0;
    uint32_t globalIndexOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;

    for (int i = 0; i < drawData->CmdListsCount; i++) {
        const ImDrawList* cmdList = drawData->CmdLists[i];
        for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdIdx];
            if (pcmd->UserCallback != nullptr) {
                pcmd->UserCallback(cmdList, pcmd);
            } else {
                // Project scissor coordinates into clipRect
                float clipMinX = pcmd->ClipRect.x - clipOff.x;
                float clipMinY = pcmd->ClipRect.y - clipOff.y;
                float clipMaxX = pcmd->ClipRect.z - clipOff.x;
                float clipMaxY = pcmd->ClipRect.w - clipOff.y;

                if (clipMinX < 0.0f) clipMinX = 0.0f;
                if (clipMinY < 0.0f) clipMinY = 0.0f;
                if (clipMaxX > drawData->DisplaySize.x) clipMaxX = drawData->DisplaySize.x;
                if (clipMaxY > drawData->DisplaySize.y) clipMaxY = drawData->DisplaySize.y;
                if (clipMaxX <= clipMinX || clipMaxY <= clipMinY)
                    continue;

                rhi::ScissorRect scissorRect = {
                    (uint32_t)clipMinX,
                    (uint32_t)clipMinY,
                    (uint32_t)clipMaxX,
                    (uint32_t)clipMaxY
                };

                // Setup state
                rhi::RenderState renderState = {};
                renderState.viewports[0] = rhi::Viewport::fromSize(drawData->DisplaySize.x, drawData->DisplaySize.y);
                renderState.viewportCount = 1;
                renderState.scissorRects[0] = scissorRect;
                renderState.scissorRectCount = 1;

                renderState.vertexBuffers[0].buffer = vertexBuffer;
                renderState.vertexBuffers[0].offset = 0;
                renderState.vertexBufferCount = 1;

                renderState.indexBuffer.buffer = indexBuffer;
                renderState.indexBuffer.offset = 0;
                renderState.indexFormat = (sizeof(ImDrawIdx) == 2) ? rhi::IndexFormat::Uint16 : rhi::IndexFormat::Uint32;

                renderPassEncoder->setRenderState(renderState);

                // Bind resources
                rhi::ShaderCursor cursor(renderPassEncoder->bindPipeline(g_Pipeline));
                
                rhi::ITextureView* texView = (rhi::ITextureView*)pcmd->GetTexID();
                if (texView) {
                    cursor["fontTexture"].setBinding(texView);
                } else {
                    cursor["fontTexture"].setBinding(g_FontTextureView);
                }
                cursor["fontSampler"].setBinding(g_FontSampler);
                cursor["uScale"].setData(&uScale, sizeof(uScale));
                cursor["uTranslate"].setData(&uTranslate, sizeof(uTranslate));
                cursor["isSrgbTarget"].setData(&g_IsSrgbTarget, sizeof(g_IsSrgbTarget));

                // Draw
                rhi::DrawArguments drawArgs = {};
                drawArgs.vertexCount = pcmd->ElemCount;
                drawArgs.startIndexLocation = pcmd->IdxOffset + globalIndexOffset;
                drawArgs.startVertexLocation = pcmd->VtxOffset + globalVertexOffset;

                renderPassEncoder->drawIndexed(drawArgs);
            }
        }
        globalVertexOffset += cmdList->VtxBuffer.Size;
        globalIndexOffset += cmdList->IdxBuffer.Size;
    }
}
