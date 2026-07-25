#include "im3d_impl_slang_rhi.h"
#include <slang-rhi/shader-cursor.h>
#include <cmath>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {
    struct PipelineSet {
        rhi::ComPtr<rhi::IRenderPipeline> trianglesPipeline;
        rhi::ComPtr<rhi::IRenderPipeline> linesPipeline;
        rhi::ComPtr<rhi::IRenderPipeline> pointsPipeline;
    };

    rhi::IDevice* g_Device = nullptr;
    rhi::ComPtr<rhi::IInputLayout> g_InputLayout;
    rhi::ComPtr<rhi::IShaderProgram> g_ShaderProgram;
    rhi::ComPtr<rhi::IShaderProgram> g_LinesShaderProgram;

    std::map<std::pair<rhi::Format, rhi::Format>, PipelineSet> g_PipelineCache;

    static const int kMaxFramesInFlight = 3;
    rhi::ComPtr<rhi::IBuffer> g_VertexBuffers[kMaxFramesInFlight];
    uint64_t g_VertexBufferSize[kMaxFramesInFlight] = {0};
    int g_FrameIndex = 0;

    rhi::Format g_DefaultColorFormat = rhi::Format::Undefined;
    rhi::Format g_DefaultDepthFormat = rhi::Format::Undefined;
    float g_LineWidthScale = 1.0f;

    static float g_CachedViewProj[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

const std::string im3d_slang_shader_src = R"(
struct VertexInput
{
    float4 positionSize : POSITION;
    uint color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float size : SIZE;
};

struct GeometryOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

uniform float4x4 uViewProj;
uniform float2 uViewport;
uniform float uLineWidthScale;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(uViewProj, float4(input.positionSize.xyz, 1.0));

    uint c = input.color;
    output.color = float4(
        float((c >> 24u) & 0xFFu),
        float((c >> 16u) & 0xFFu),
        float((c >> 8u)  & 0xFFu),
        float(c & 0xFFu)
    ) / 255.0;

    output.size = max(1.0, input.positionSize.w * uLineWidthScale);
    return output;
}

[maxvertexcount(4)]
[shader("geometry")]
void linesGeom(line VertexOutput input[2], inout TriangleStream<GeometryOutput> outputStream)
{
    float4 pos0 = input[0].position;
    float4 pos1 = input[1].position;

    float2 screen0 = (pos0.xy / pos0.w) * uViewport * 0.5;
    float2 screen1 = (pos1.xy / pos1.w) * uViewport * 0.5;

    float2 lineVector = screen1 - screen0;
    float len = length(lineVector);
    
    float2 lineNormal = (len > 0.0001) ? float2(-lineVector.y, lineVector.x) / len : float2(0.0, 1.0);

    float halfWidth0 = input[0].size * 0.5;
    float halfWidth1 = input[1].size * 0.5;

    float2 delta0 = (lineNormal * halfWidth0 / (uViewport * 0.5)) * pos0.w;
    float2 delta1 = (lineNormal * halfWidth1 / (uViewport * 0.5)) * pos1.w;

    GeometryOutput output;

    output.position = float4(pos0.xy + delta0, pos0.zw);
    output.color = input[0].color;
    outputStream.Append(output);

    output.position = float4(pos0.xy - delta0, pos0.zw);
    output.color = input[0].color;
    outputStream.Append(output);

    output.position = float4(pos1.xy + delta1, pos1.zw);
    output.color = input[1].color;
    outputStream.Append(output);

    output.position = float4(pos1.xy - delta1, pos1.zw);
    output.color = input[1].color;
    outputStream.Append(output);

    outputStream.RestartStrip();
}

[shader("fragment")]
float4 fragmentMain(GeometryOutput input) : SV_Target
{
    return input.color;
}
)";

static PipelineSet CreatePipelineSetForFormats(rhi::Format colorFormat, rhi::Format depthFormat) {
    PipelineSet set = {};
    if (!g_Device || !g_ShaderProgram || !g_InputLayout) return set;

    rhi::ColorTargetDesc colorTarget = {};
    colorTarget.format = colorFormat;
    colorTarget.enableBlend = true;
    colorTarget.color.srcFactor = rhi::BlendFactor::SrcAlpha;
    colorTarget.color.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    colorTarget.color.op = rhi::BlendOp::Add;
    colorTarget.alpha.srcFactor = rhi::BlendFactor::One;
    colorTarget.alpha.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    colorTarget.alpha.op = rhi::BlendOp::Add;

    rhi::DepthStencilDesc depthStencilDesc = {};
    if (depthFormat != rhi::Format::Undefined) {
        depthStencilDesc.format = depthFormat;
        depthStencilDesc.depthTestEnable = true;
        depthStencilDesc.depthWriteEnable = true;
        depthStencilDesc.depthFunc = rhi::ComparisonFunc::LessEqual;
    } else {
        depthStencilDesc.depthTestEnable = false;
        depthStencilDesc.depthWriteEnable = false;
    }

    rhi::RasterizerDesc rasterizerDesc = {};
    rasterizerDesc.fillMode = rhi::FillMode::Solid;
    rasterizerDesc.cullMode = rhi::CullMode::None;
    rasterizerDesc.scissorEnable = false;

    rhi::RenderPipelineDesc pipelineDesc = {};
    pipelineDesc.program = g_ShaderProgram;
    pipelineDesc.inputLayout = g_InputLayout;
    pipelineDesc.targets = &colorTarget;
    pipelineDesc.targetCount = 1;
    pipelineDesc.depthStencil = depthStencilDesc;
    pipelineDesc.rasterizer = rasterizerDesc;

    pipelineDesc.primitiveTopology = rhi::PrimitiveTopology::TriangleList;
    pipelineDesc.label = "Im3d_Triangles_Pipeline";
    if (SLANG_FAILED(g_Device->createRenderPipeline(pipelineDesc, set.trianglesPipeline.writeRef()))) {
        std::cerr << "Im3d backend: Failed to create triangles pipeline" << std::endl;
    }

    pipelineDesc.program = g_LinesShaderProgram ? g_LinesShaderProgram.get() : g_ShaderProgram.get();
    pipelineDesc.primitiveTopology = rhi::PrimitiveTopology::LineList;
    pipelineDesc.label = "Im3d_Lines_Pipeline";
    if (SLANG_FAILED(g_Device->createRenderPipeline(pipelineDesc, set.linesPipeline.writeRef()))) {
        std::cerr << "Im3d backend: Failed to create lines pipeline" << std::endl;
    }

    pipelineDesc.program = g_ShaderProgram;
    pipelineDesc.primitiveTopology = rhi::PrimitiveTopology::PointList;
    pipelineDesc.label = "Im3d_Points_Pipeline";
    if (SLANG_FAILED(g_Device->createRenderPipeline(pipelineDesc, set.pointsPipeline.writeRef()))) {
        std::cerr << "Im3d backend: Failed to create points pipeline" << std::endl;
    }

    return set;
}

static const PipelineSet& GetOrCreatePipelineSet(rhi::Format colorFormat, rhi::Format depthFormat) {
    auto key = std::make_pair(colorFormat, depthFormat);
    auto it = g_PipelineCache.find(key);
    if (it != g_PipelineCache.end()) {
        return it->second;
    }

    PipelineSet newSet = CreatePipelineSetForFormats(colorFormat, depthFormat);
    auto res = g_PipelineCache.emplace(key, std::move(newSet));
    return res.first->second;
}

void Im3d_ImplSlangRHI_SetLineWidthScale(float scale) {
    g_LineWidthScale = (scale > 0.0f) ? scale : 1.0f;
}

float Im3d_ImplSlangRHI_GetLineWidthScale() {
    return g_LineWidthScale;
}

bool Im3d_ImplSlangRHI_Init(rhi::IDevice* device, rhi::Format defaultRenderTargetFormat, rhi::Format defaultDepthFormat) {
    g_Device = device;
    g_FrameIndex = 0;
    g_DefaultColorFormat = defaultRenderTargetFormat;
    g_DefaultDepthFormat = defaultDepthFormat;
    g_LineWidthScale = 1.0f;

    rhi::ComPtr<slang::IBlob> diagnostics;
    slang::IModule* module = g_Device->getSlangSession()->loadModule("resources/shaders/im3d", diagnostics.writeRef());
    if (!module) {
        diagnostics = nullptr;
        module = g_Device->getSlangSession()->loadModuleFromSourceString(
            "im3d_builtin", "im3d_builtin.slang", im3d_slang_shader_src.c_str(), diagnostics.writeRef());
    }

    if (diagnostics) {
        std::cout << "Im3d Shader compilation messages:\n" << (const char*)diagnostics->getBufferPointer() << std::endl;
    }
    if (!module) {
        std::cerr << "Im3d backend: Failed to load im3d shader module" << std::endl;
        return false;
    }

    slang::IEntryPoint* vertexEntryPoint = nullptr;
    module->findEntryPointByName("vertexMain", &vertexEntryPoint);
    slang::IEntryPoint* fragmentEntryPoint = nullptr;
    module->findEntryPointByName("fragmentMain", &fragmentEntryPoint);
    slang::IEntryPoint* geometryEntryPoint = nullptr;
    module->findEntryPointByName("linesGeom", &geometryEntryPoint);

    std::vector<slang::IComponentType*> entryPoints = { vertexEntryPoint, fragmentEntryPoint };

    rhi::ShaderProgramDesc programDesc = {};
    programDesc.linkingStyle = rhi::LinkingStyle::SingleProgram;
    programDesc.slangEntryPoints = entryPoints.data();
    programDesc.slangEntryPointCount = (uint32_t)entryPoints.size();
    programDesc.slangGlobalScope = module;

    if (SLANG_FAILED(g_Device->createShaderProgram(programDesc, g_ShaderProgram.writeRef(), diagnostics.writeRef()))) {
        std::cerr << "Im3d backend: Failed to create shader program" << std::endl;
        if (diagnostics) {
            std::cerr << (const char*)diagnostics->getBufferPointer() << std::endl;
        }
        return false;
    }

    if (geometryEntryPoint) {
        std::vector<slang::IComponentType*> linesEntryPoints = { vertexEntryPoint, geometryEntryPoint, fragmentEntryPoint };
        rhi::ShaderProgramDesc linesProgramDesc = {};
        linesProgramDesc.linkingStyle = rhi::LinkingStyle::SingleProgram;
        linesProgramDesc.slangEntryPoints = linesEntryPoints.data();
        linesProgramDesc.slangEntryPointCount = (uint32_t)linesEntryPoints.size();
        linesProgramDesc.slangGlobalScope = module;

        if (SLANG_FAILED(g_Device->createShaderProgram(linesProgramDesc, g_LinesShaderProgram.writeRef(), diagnostics.writeRef()))) {
            g_LinesShaderProgram = g_ShaderProgram;
        }
    } else {
        g_LinesShaderProgram = g_ShaderProgram;
    }

    rhi::VertexStreamDesc vertexStreams[] = {
        {sizeof(Im3d::VertexData), rhi::InputSlotClass::PerVertex, 0},
    };
    rhi::InputElementDesc inputElements[] = {
        {"POSITION", 0, rhi::Format::RGBA32Float, offsetof(Im3d::VertexData, m_positionSize), 0},
        {"COLOR",    0, rhi::Format::R32Uint,     offsetof(Im3d::VertexData, m_color), 0},
    };
    rhi::InputLayoutDesc inputLayoutDesc = {};
    inputLayoutDesc.inputElements = inputElements;
    inputLayoutDesc.inputElementCount = sizeof(inputElements) / sizeof(inputElements[0]);
    inputLayoutDesc.vertexStreams = vertexStreams;
    inputLayoutDesc.vertexStreamCount = sizeof(vertexStreams) / sizeof(vertexStreams[0]);

    if (SLANG_FAILED(g_Device->createInputLayout(inputLayoutDesc, g_InputLayout.writeRef()))) {
        std::cerr << "Im3d backend: Failed to create input layout" << std::endl;
        return false;
    }

    if (defaultRenderTargetFormat != rhi::Format::Undefined) {
        GetOrCreatePipelineSet(defaultRenderTargetFormat, defaultDepthFormat);
    }

    return true;
}

void Im3d_ImplSlangRHI_Shutdown() {
    g_PipelineCache.clear();
    g_ShaderProgram = nullptr;
    g_LinesShaderProgram = nullptr;
    g_InputLayout = nullptr;
    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        g_VertexBuffers[i] = nullptr;
        g_VertexBufferSize[i] = 0;
    }
    g_Device = nullptr;
}

void Im3d_ImplSlangRHI_NewFrame(float deltaTime,
                                float viewportWidth,
                                float viewportHeight,
                                const float* viewMatrix4x4,
                                const float* projMatrix4x4,
                                const float* camPos3,
                                const float* camDir3,
                                float fovY) {
    Im3d::AppData& appData = Im3d::GetAppData();
    appData.m_deltaTime = deltaTime;
    appData.m_viewportSize = Im3d::Vec2(viewportWidth, viewportHeight);
    
    if (camPos3) {
        appData.m_viewOrigin = Im3d::Vec3(camPos3[0], camPos3[1], camPos3[2]);
    } else {
        appData.m_viewOrigin = Im3d::Vec3(0.0f, 0.0f, 0.0f);
    }

    if (camDir3) {
        appData.m_viewDirection = Im3d::Vec3(camDir3[0], camDir3[1], camDir3[2]);
    } else {
        appData.m_viewDirection = Im3d::Vec3(0.0f, 0.0f, -1.0f);
    }

    appData.m_projOrtho = false;
    appData.m_projScaleY = std::tan(fovY * 0.5f);

    if (viewMatrix4x4 && projMatrix4x4) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += projMatrix4x4[k * 4 + r] * viewMatrix4x4[c * 4 + k];
                }
                g_CachedViewProj[c * 4 + r] = sum;
            }
        }
    }

    Im3d::NewFrame();
}

void Im3d_ImplSlangRHI_RenderDrawData(rhi::ICommandEncoder* commandEncoder,
                                      rhi::IRenderPassEncoder* renderPassEncoder,
                                      float viewportWidth,
                                      float viewportHeight,
                                      const float* viewProjMatrix4x4,
                                      rhi::Format colorFormat,
                                      rhi::Format depthFormat,
                                      float lineWidthScale) {
    Im3d::EndFrame();

    uint32_t drawListCount = Im3d::GetDrawListCount();
    if (drawListCount == 0) return;

    if (colorFormat == rhi::Format::Undefined) {
        colorFormat = g_DefaultColorFormat;
    }
    if (depthFormat == rhi::Format::Undefined) {
        depthFormat = g_DefaultDepthFormat;
    }

    if (colorFormat == rhi::Format::Undefined) {
        std::cerr << "Im3d backend error: Render target format not specified" << std::endl;
        return;
    }

    const PipelineSet& pipelines = GetOrCreatePipelineSet(colorFormat, depthFormat);

    uint32_t totalVertices = 0;
    const Im3d::DrawList* drawLists = Im3d::GetDrawLists();
    for (uint32_t i = 0; i < drawListCount; ++i) {
        totalVertices += drawLists[i].m_vertexCount;
    }

    if (totalVertices == 0) return;

    uint64_t requiredBufferSize = totalVertices * sizeof(Im3d::VertexData);
    if (!g_VertexBuffers[g_FrameIndex] || g_VertexBufferSize[g_FrameIndex] < requiredBufferSize) {
        g_VertexBufferSize[g_FrameIndex] = requiredBufferSize + 1024 * sizeof(Im3d::VertexData);
        rhi::BufferDesc bufferDesc = {};
        bufferDesc.size = g_VertexBufferSize[g_FrameIndex];
        bufferDesc.usage = rhi::BufferUsage::VertexBuffer;
        bufferDesc.defaultState = rhi::ResourceState::VertexBuffer;
        bufferDesc.memoryType = rhi::MemoryType::Upload;
        bufferDesc.label = "Im3d_VertexBuffer";

        if (SLANG_FAILED(g_Device->createBuffer(bufferDesc, nullptr, g_VertexBuffers[g_FrameIndex].writeRef()))) {
            std::cerr << "Im3d backend: Failed to create vertex buffer" << std::endl;
            return;
        }
    }

    void* mappedData = nullptr;
    if (SLANG_FAILED(g_Device->mapBuffer(g_VertexBuffers[g_FrameIndex], rhi::CpuAccessMode::Write, &mappedData))) {
        std::cerr << "Im3d backend: Failed to map vertex buffer" << std::endl;
        return;
    }

    uint8_t* bytePtr = (uint8_t*)mappedData;
    uint32_t vertexOffset = 0;
    for (uint32_t i = 0; i < drawListCount; ++i) {
        const auto& dl = drawLists[i];
        size_t listBytes = dl.m_vertexCount * sizeof(Im3d::VertexData);
        std::memcpy(bytePtr + vertexOffset * sizeof(Im3d::VertexData), dl.m_vertexData, listBytes);
        vertexOffset += dl.m_vertexCount;
    }

    g_Device->unmapBuffer(g_VertexBuffers[g_FrameIndex]);

    const float* vpMatrix = viewProjMatrix4x4 ? viewProjMatrix4x4 : g_CachedViewProj;
    float lineScaleToUse = (lineWidthScale > 0.0f) ? lineWidthScale : g_LineWidthScale;

    rhi::RenderState renderState = {};
    renderState.viewports[0] = rhi::Viewport::fromSize(viewportWidth, viewportHeight);
    renderState.viewportCount = 1;
    renderState.vertexBuffers[0].buffer = g_VertexBuffers[g_FrameIndex];
    renderState.vertexBuffers[0].offset = 0;
    renderState.vertexBufferCount = 1;

    renderPassEncoder->setRenderState(renderState);

    uint32_t currentVertexStart = 0;
    for (uint32_t i = 0; i < drawListCount; ++i) {
        const auto& dl = drawLists[i];
        if (dl.m_vertexCount == 0) continue;

        rhi::IRenderPipeline* pipelineToBind = nullptr;
        switch (dl.m_primType) {
            case Im3d::DrawPrimitive_Triangles:
                pipelineToBind = pipelines.trianglesPipeline;
                break;
            case Im3d::DrawPrimitive_Lines:
                pipelineToBind = pipelines.linesPipeline;
                break;
            case Im3d::DrawPrimitive_Points:
                pipelineToBind = pipelines.pointsPipeline;
                break;
            default:
                break;
        }

        if (pipelineToBind) {
            rhi::ShaderCursor cursor(renderPassEncoder->bindPipeline(pipelineToBind));
            cursor["uViewProj"].setData(vpMatrix, sizeof(float) * 16);
            float viewport[2] = { viewportWidth, viewportHeight };
            cursor["uViewport"].setData(viewport, sizeof(float) * 2);
            cursor["uLineWidthScale"].setData(&lineScaleToUse, sizeof(lineScaleToUse));

            rhi::DrawArguments drawArgs = {};
            drawArgs.vertexCount = dl.m_vertexCount;
            drawArgs.startVertexLocation = currentVertexStart;
            renderPassEncoder->draw(drawArgs);
        }

        currentVertexStart += dl.m_vertexCount;
    }

    g_FrameIndex = (g_FrameIndex + 1) % kMaxFramesInFlight;
}
