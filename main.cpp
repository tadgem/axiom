#include "axiom.hpp"
#include <array>

#include "slang-rhi/shader-cursor.h"
using namespace axm;

int main() {
    AppState init = engine::Init();

    AXM_ASSERT(init.m_OK, "Failed to start AXIOM");

    mat4 model  = maths::Multiply(
        maths::RotateX(maths::Radians(0.016f)),
        maths::RotateY(maths::Radians(0.016f))
    );
    mat4 view   = maths::Translate(vec3 { 0.0f, 0.0f, -2.5f });
    mat4 proj   = maths::PerspectiveFOV(maths::Radians(45.0f), 1.666f, 0.1f, 10.0f);

    auto modelView     = maths::Multiply(view, model);
    auto mvp    = maths::Multiply(proj, modelView);

    std::array formats =
    {
        init.m_Surface->getInfo().preferredFormat
    };

    constexpr u32 width = 1280;
    constexpr u32 height = 720;

    auto cubeShapeDef = shapes::GetCubeShape();

    auto vertexBuffer = buffer::CreateVertexBuffer(
        init.m_Device,
        cubeShapeDef.m_BufferLength * sizeof(f32),
        cubeShapeDef.m_VertexBuffer,
        "CubeVertexBuffer_PosNormalUV"
    );

    auto indexBuffer = buffer::CreateIndexBuffer(
        init.m_Device,
        cubeShapeDef.m_NumIndices * sizeof(i32),
        cubeShapeDef.m_IndexBuffer, "CubeIndexBuffer"
    );


    Shader cube = Shader(init.m_Device, "resources/cube");
    auto pipeline = pipeline::CreateRasterPipeline(
        init.m_Device,
        formats,
        init.m_DepthStencilDesc,
        cube,
        vertex::PosNormalUV::GetInputLayout(init.m_Device));

    AXM_LOG("Starting Axiom Main Loop");
    AXM_FLUSH_LOG();

    while (init.m_Running) {
        engine::PreFrame(init);

        auto commandEncoder = init.m_Queue->createCommandEncoder();
        auto renderPassEncoder = render_pass::BeginSwapChainRenderPass(init, commandEncoder);

        auto cursor = rhi::ShaderCursor(renderPassEncoder->bindPipeline(pipeline));
        if (SLANG_FAILED(cursor["modelViewProj"].setData(&mvp, sizeof(mat4)))) {
            AXM_LOG("Failed to bind modelViewProj to pipeline");
        }

        rhi::RenderState renderState = {};
        renderState.viewports[0] = rhi::Viewport::fromSize(width, height);
        renderState.viewportCount = 1;
        renderState.scissorRects[0] = rhi::ScissorRect::fromSize(width, height);
        renderState.scissorRectCount = 1;
        renderState.vertexBuffers[0].buffer = vertexBuffer;
        renderState.vertexBufferCount = 1;
        renderState.indexBuffer.buffer = indexBuffer;
        renderState.indexFormat = rhi::IndexFormat::Uint32;
        renderPassEncoder->setRenderState(renderState);

        rhi::DrawArguments drawArgs = {};
        drawArgs.vertexCount = cubeShapeDef.m_NumIndices;
        renderPassEncoder->drawIndexed(drawArgs);

        if (ImGui::Begin("Hello!")) {

        }
        ImGui::End();
        ImGui::Render();

        renderPassEncoder->end();
        init.m_Queue->submit(commandEncoder->finish());

        engine::PostFrame(init);
    }

    engine::Quit(init);

}
