#include "Engine.hpp"
#include "Log.hpp"
#define SDL_MAIN_HANDLED
#include "SDL3/SDL_main.h"
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

    ITexture* depthTexture = Utils::CreateDepthTexture(device, 1280, 720);


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

    return {
        .m_OK = true,
        .m_Window = window,
        .m_Device = device,
        .m_Surface = surface,
        .m_Queue = graphicsQueue
    };
}

void axm::engine::Quit(const AppState &init) {
    init.m_Queue->waitOnHost();

    ImGui_ImplSlangRHI_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(init.m_Window);
    SDL_Quit();
}

axm::AppState axm::AppState::BAD() {
    return {
        .m_OK =  false,
        .m_Window = nullptr,
        .m_Device = nullptr,
        .m_Surface = nullptr
    };
}
