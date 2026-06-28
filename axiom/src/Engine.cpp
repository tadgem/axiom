#include "Engine.hpp"
#include "Debug.hpp"
#include "Utils.hpp"
#include "STL.hpp"

#define SDL_MAIN_HANDLED


#include <slang-rhi.h>
#include <slang.h>
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_slang_rhi.h"
#include "imgui.h"
#if SLANG_WINDOWS_FAMILY
#include "windows.h"
#endif

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

constexpr const char* UNKNOWN_MSG = "UNKNOWN";

const char* GetSlangRHIDebugMessageType(const rhi::DebugMessageType& messageType) {
    constexpr const char* INFO_MSG = "INFO";
    constexpr const char* WARN_MSG = "WARN";
    constexpr const char* ERROR_MSG = "ERROR";

    switch (messageType) {
        case rhi::DebugMessageType::Info: return INFO_MSG;
        case rhi::DebugMessageType::Warning: return WARN_MSG;
        case rhi::DebugMessageType::Error: return ERROR_MSG;
        default: return UNKNOWN_MSG;
    }
}

const char* GetSlangRHIDebugMessageSource(const rhi::DebugMessageSource& messageType) {
    constexpr const char* LAYER_MSG = "LAYER";
    constexpr const char* DRIVER_MSG = "DRIVER";
    constexpr const char* SLANG_MSG = "SLANG";

    switch (messageType) {
        case rhi::DebugMessageSource::Layer: return LAYER_MSG;
        case rhi::DebugMessageSource::Driver: return DRIVER_MSG;
        case rhi::DebugMessageSource::Slang: return SLANG_MSG;
        default: return UNKNOWN_MSG;
    }
}


class SlangRHIDebugCallback : public rhi::IDebugCallback
{
public:
    SLANG_NO_THROW void SLANG_MCALL handleMessage(
        rhi::DebugMessageType type,
        rhi::DebugMessageSource source,
        const char* message
    ) override
    {
        AXM_LOG("SlangRHI : {} : {} : {}",
            GetSlangRHIDebugMessageType(type),
            GetSlangRHIDebugMessageSource(source),
            message
        );
    }

    virtual ~SlangRHIDebugCallback() = default;
};

axm::AppState axm::engine::Init() {
    using namespace rhi;
    SDL_SetMainReady();

    constexpr SDL_InitFlags kInitFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

    if (!SDL_Init(kInitFlags)) {
        AXM_LOG("Failed to initialize AXIOM : SDL Init failed.");
        return AppState::BAD();
    }

    SDL_Window* window = SDL_CreateWindow("AXIOM", 1280, 720, /*SDL_WINDOW_RESIZABLE*/ 0);
    if (!window) {
       AXM_LOG("Failed to initialize AXIOM : SDL Window Creation failed.");
        SDL_Quit();
        return AppState::BAD();
    }

    IDevice* device;
    DeviceType deviceTypes[] = { DeviceType::Vulkan, DeviceType::D3D11, DeviceType::D3D12, DeviceType::Metal };
    DeviceType selectedType = DeviceType::Default;


    IDebugCallback* dbg = AXM_NEW(SlangRHIDebugCallback);
    Unique<IDebugCallback> debugCallback (dbg);

    for (auto type : deviceTypes) {
        if (getRHI()->isDeviceTypeSupported(type)) {
            DeviceDesc deviceDesc = {};
            deviceDesc.deviceType = type;
            deviceDesc.debugCallback = debugCallback.get();
            deviceDesc.enableValidation = true;

            Array<Feature, 2> requiredFeatures = { Feature::Surface, Feature::Rasterization };
            deviceDesc.requiredFeatureCount = static_cast<uint32_t>(requiredFeatures.size());
            deviceDesc.requiredFeatures = requiredFeatures.data();

            if (SLANG_SUCCEEDED(getRHI()->createDevice(deviceDesc, &device))) {
                AXM_LOG("Selected Rendering Backend : {}", getRHI()->getDeviceTypeName(type));
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

    DepthStencilDesc depthStencilDesc = {};
    depthStencilDesc.format = Format::D32Float;
    depthStencilDesc.depthTestEnable = true;
    depthStencilDesc.depthWriteEnable = true;
    depthStencilDesc.depthFunc = ComparisonFunc::LessEqual;


    return {
        .m_OK = true,
        .m_Running = true,
        .m_AssetManager = AssetManager(),
        .m_DepthStencilDesc =  depthStencilDesc,
        .m_Window = window,
        .m_Device = device,
        .m_Surface = surface,
        .m_Queue = graphicsQueue,
        .m_SwapchainColourImage = nullptr,
        .m_SwapchainDepthImage = depthTexture,
        .m_DebugCallback = std::move(debugCallback)
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
    auto passEncoder = BeginSwapchainRenderPass(e, commandEncoder, rhi::LoadOp::Load);

    ImGui_ImplSlangRHI_RenderDrawData(ImGui::GetDrawData(), commandEncoder, passEncoder);

    passEncoder->end();
    e.m_Queue->submit(commandEncoder->finish());
    e.m_Surface->present();

    AXM_FLUSH_LOG();
}

rhi::IRenderPassEncoder *axm::engine::BeginSwapchainRenderPass(AppState& e, rhi::ICommandEncoder* cmd, rhi::LoadOp loadOp) {
    rhi::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = e.m_SwapchainColourImage->getDefaultView();
    colorAttachment.loadOp = loadOp;
    colorAttachment.storeOp = rhi::StoreOp::Store;
    colorAttachment.clearValue[0] = 0.0f;
    colorAttachment.clearValue[1] = 0.0f;
    colorAttachment.clearValue[2] = 0.0f;
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
        .m_AssetManager = AssetManager(),
        .m_DepthStencilDesc = {},
        .m_Window = nullptr,
        .m_Device = nullptr,
        .m_Surface = nullptr,
        .m_Queue = nullptr,
        .m_SwapchainColourImage = nullptr,
        .m_SwapchainDepthImage = nullptr
    };
}
