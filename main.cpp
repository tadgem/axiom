#include <print>
#include "Init.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_slang_rhi.h"

int main() {
    std::println("AXIOM");
    axm::InitResult init = axm::init::Init();

    bool running = true;
    auto depthTexture =
        axm::Utils::CreateDepthTexture(init.m_Device, 1280, 720, rhi::Format::D32Float);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                default:
                    // AXM_LOG("Unhandled event");
                    break;
            }
        }

        ImGui_ImplSlangRHI_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Hello!")) {

        }
        ImGui::End();
        ImGui::Render();

        rhi::ITexture* colorImage;
        init.m_Surface->acquireNextImage(&colorImage);


        auto commandEncoder = init.m_Queue->createCommandEncoder();

        rhi::RenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = colorImage->getDefaultView();
        colorAttachment.loadOp = rhi::LoadOp::Clear;
        colorAttachment.storeOp = rhi::StoreOp::Store;
        colorAttachment.clearValue[0] = 0.15f;
        colorAttachment.clearValue[1] = 0.1f;
        colorAttachment.clearValue[2] = 0.1f;
        colorAttachment.clearValue[3] = 1.0f;

        rhi::RenderPassDepthStencilAttachment depthAttachment = {};
        depthAttachment.view = depthTexture->getDefaultView();
        depthAttachment.depthLoadOp = rhi::LoadOp::Clear;
        depthAttachment.depthStoreOp = rhi::StoreOp::Store;
        depthAttachment.depthClearValue = 1.0f;

        rhi::RenderPassDesc renderPass = {};
        renderPass.colorAttachments = &colorAttachment;
        renderPass.colorAttachmentCount = 1;
        renderPass.depthStencilAttachment = &depthAttachment;

        rhi::IRenderPassEncoder* passEncoder = commandEncoder->beginRenderPass(renderPass);

        ImGui_ImplSlangRHI_RenderDrawData(ImGui::GetDrawData(), commandEncoder, passEncoder);

        passEncoder->end();
        init.m_Queue->submit(commandEncoder->finish());
        init.m_Surface->present();
    }


    axm::init::Quit(init);

}
