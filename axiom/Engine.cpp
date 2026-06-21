#include "Engine.hpp"
#include "Log.hpp"
#define SDL_MAIN_HANDLED
#include "SDL3/SDL_main.h"
#include "SDL3/SDL.h"
#include <slang.h>
#include <slang-rhi.h>
#include <vector>

#include "Utils.hpp"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_slang_rhi.h"
#include "imgui.h"
#include "windows.h"
inline rhi::WindowHandle _getWindowHandleFromSDL(SDL_Window* window)
{
#if SLANG_WINDOWS_FAMILY
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    return rhi::WindowHandle::fromHwnd(hwnd);
#elif SLANG_LINUX_FAMILY
    AXM_LOG("No Linux Window Handling yet");
    return {};
#elif SLANG_APPLE_FAMILY
    HWND hwnd =
    id nswindow = (id)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    return WindowHandle::fromNSWindow(nswindow);
#endif
    return {};
}
axm::AppState axm::engine::Init() {
    using namespace rhi;
    SDL_SetMainReady();

    constexpr SDL_InitFlags kInitFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

    if (!SDL_Init(kInitFlags)) {
        AXM_LOG("Failed to initialize AXIOM : SDL Init failed.");
        return AppState::BAD();
    }

    SDL_Window* window = SDL_CreateWindow("AXIOM", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
       AXM_LOG("Failed to initialize AXIOM : SDL Window Creation failed.");
        SDL_Quit();
        return AppState::BAD();
    }

    IDevice* device;
    DeviceType deviceTypes[] = { DeviceType::Vulkan, DeviceType::D3D12, DeviceType::D3D11, DeviceType::Metal };
    DeviceType selectedType = DeviceType::Default;


    for (auto type : deviceTypes) {
        if (getRHI()->isDeviceTypeSupported(type)) {
            DeviceDesc deviceDesc = {};
            deviceDesc.deviceType = type;

            std::vector<Feature> requiredFeatures = { Feature::Surface, Feature::Rasterization };
            deviceDesc.requiredFeatureCount = static_cast<uint32_t>(requiredFeatures.size());
            deviceDesc.requiredFeatures = requiredFeatures.data();

            if (SLANG_SUCCEEDED(getRHI()->createDevice(deviceDesc, &device))) {
                selectedType = type;
                break;
            }
        }
    }

    if (!device) {
        AXM_LOG("Failed to create slang-rhi device");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return {};
    }

    int width = 800, height = 600;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    ISurface* surface;
    if (SLANG_FAILED(device->createSurface(_getWindowHandleFromSDL(window), &surface))) {
        AXM_LOG("Failed to create surface from native window handle");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return {};
    }

    SurfaceConfig surfaceConfig = {};
    surfaceConfig.width = width;
    surfaceConfig.height = height;
    surfaceConfig.format = Format::Undefined;
    if (SLANG_FAILED(surface->configure(surfaceConfig))) {
        AXM_LOG("Failed to configure surface");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return {};
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOther(window);
    if (!ImGui_ImplSlangRHI_Init(device, surface->getInfo().preferredFormat)) {
        AXM_LOG("Failed to initialize ImGui Slang RHI backend");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return {};
    }

    // Get command queue
    ICommandQueue* graphicsQueue;
    if (SLANG_FAILED(device->getQueue(QueueType::Graphics, &graphicsQueue))) {
        AXM_LOG("Failed to get Graphics queue");
        return {};
    }

    ITexture* depthTexture = Utils::CreateDepthTexture(device, 1280, 720);

    return {
        .m_OK = true,
        .m_Window = window,
        .m_Device = device,
        .m_Surface = surface,
        .m_Queue = graphicsQueue,
        .m_SwapchainColourImage = nullptr,
        .m_SwapchainDepthImage = depthTexture
    };
}

void axm::engine::Quit(const AppState &e) {
    e.m_Queue->waitOnHost();

    ImGui_ImplSlangRHI_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(e.m_Window);
    SDL_Quit();
}
void axm::engine::PreFrame(AppState &e) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                e.m_Running = false;
                break;
            default:
                // AXM_LOG("Unhandled event");
                break;
        }
    }

    ImGui_ImplSlangRHI_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    e.m_Surface->acquireNextImage(&e.m_SwapchainColourImage);
}

void axm::engine::PostFrame(AppState &e) {
    auto commandEncoder = e.m_Queue->createCommandEncoder();
    auto passEncoder = engine::BeginSwapchainRenderPass(e, commandEncoder, rhi::LoadOp::Load);

    ImGui_ImplSlangRHI_RenderDrawData(ImGui::GetDrawData(), commandEncoder, passEncoder);

    passEncoder->end();
    e.m_Queue->submit(commandEncoder->finish());
    e.m_Surface->present();
}

rhi::IRenderPassEncoder *axm::engine::BeginSwapchainRenderPass(AppState& e, rhi::ICommandEncoder* cmd, rhi::LoadOp loadOp) {
    rhi::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = e.m_SwapchainColourImage->getDefaultView();
    colorAttachment.loadOp = rhi::LoadOp::Clear;
    colorAttachment.storeOp = rhi::StoreOp::Store;
    colorAttachment.clearValue[0] = 0.15f;
    colorAttachment.clearValue[1] = 0.1f;
    colorAttachment.clearValue[2] = 0.1f;
    colorAttachment.clearValue[3] = 1.0f;

    rhi::RenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.view = e.m_SwapchainDepthImage->getDefaultView();
    depthAttachment.depthLoadOp = rhi::LoadOp::Clear;
    depthAttachment.depthStoreOp = rhi::StoreOp::Store;
    depthAttachment.depthClearValue = 1.0f;

    rhi::RenderPassDesc renderPass = {};
    renderPass.colorAttachments = &colorAttachment;
    renderPass.colorAttachmentCount = 1;
    renderPass.depthStencilAttachment = &depthAttachment;

    return cmd->beginRenderPass(renderPass);
}

axm::AppState axm::AppState::BAD() {
    return {
        .m_OK =  false,
        .m_Running = false,
        .m_Window = nullptr,
        .m_Device = nullptr,
        .m_Surface = nullptr,
        .m_Queue = nullptr,
        .m_SwapchainColourImage = nullptr,
        .m_SwapchainDepthImage = nullptr
    };
}
