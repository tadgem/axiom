#include "Core/Engine.hpp"
#include "ArchivoRegularTTF.h"
#include "Core/Debug.hpp"
#include "Core/STL.hpp"
#include "Core/Utils.hpp"
#include "Render/Pipeline.hpp"
#define SDL_MAIN_HANDLED


#include <slang-rhi.h>
#include <slang.h>
#include "Core/Profile.hpp"
#include "Render/NanoVGUtils.hpp"
#include "Render/RenderPass.hpp"
#include "Render/Texture.hpp"
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_slang_rhi.h"
#include "im3d_impl_slang_rhi.h"
#include "imgui.h"
#include "nanovg.h"
#if SLANG_WINDOWS_FAMILY
#include "windows.h"
#endif

inline rhi::WindowHandle GetNativeWindowHandle(SDL_Window* window) {
    PROFILE_SCOPE()

#if SLANG_WINDOWS_FAMILY
    HWND hwnd = (HWND) SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    return rhi::WindowHandle::fromHwnd(hwnd);
#elif SLANG_LINUX_FAMILY
    AXM_LOG("No Linux Window Handling yet");
    return { };
#elif SLANG_APPLE_FAMILY
    HWND hwnd = id nswindow = (id) SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    return WindowHandle::fromNSWindow(nswindow);
#endif
}

constexpr const char* UNKNOWN_MSG = "UNKNOWN";

const char*           GetSlangRHIDebugMessageType(const rhi::DebugMessageType& messageType) {
    PROFILE_SCOPE()

    constexpr const char* INFO_MSG  = "INFO";
    constexpr const char* WARN_MSG  = "WARN";
    constexpr const char* ERROR_MSG = "ERROR";

    switch (messageType) {
        case rhi::DebugMessageType::Info:
            return INFO_MSG;
        case rhi::DebugMessageType::Warning:
            return WARN_MSG;
        case rhi::DebugMessageType::Error:
            return ERROR_MSG;
        default:
            return UNKNOWN_MSG;
    }
}

const char* GetSlangRHIDebugMessageSource(const rhi::DebugMessageSource& messageType) {
    PROFILE_SCOPE()

    constexpr const char* LAYER_MSG  = "LAYER";
    constexpr const char* DRIVER_MSG = "DRIVER";
    constexpr const char* SLANG_MSG  = "SLANG";

    switch (messageType) {
        case rhi::DebugMessageSource::Layer:
            return LAYER_MSG;
        case rhi::DebugMessageSource::Driver:
            return DRIVER_MSG;
        case rhi::DebugMessageSource::Slang:
            return SLANG_MSG;
        default:
            return UNKNOWN_MSG;
    }
}


class SlangRHIDebugCallback : public rhi::IDebugCallback
{
public:
    SLANG_NO_THROW void SLANG_MCALL handleMessage(rhi::DebugMessageType   type,
                                                  rhi::DebugMessageSource source,
                                                  const char*             message) override {
        PROFILE_SCOPE()
        AXM_LOG("SlangRHI : {} : {} : {}",
                GetSlangRHIDebugMessageType(type),
                GetSlangRHIDebugMessageSource(source),
                message);
    }

    virtual ~SlangRHIDebugCallback() = default;
};

axm::AxiomEngine axm::engine::Init() {
    using namespace rhi;
    PROFILE_SCOPE()

    SDL_SetMainReady();

    constexpr SDL_InitFlags kInitFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

    if (!SDL_Init(kInitFlags)) {
        AXM_LOG("Failed to initialize AXIOM : SDL Init failed.");
        return AxiomEngine::BAD();
    }

    SDL_Window* window = SDL_CreateWindow("AXIOM", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        AXM_LOG("Failed to initialize AXIOM : SDL Window Creation failed.");
        SDL_Quit();
        return AxiomEngine::BAD();
    }

    IDevice*        device        = nullptr;
    DeviceType      deviceTypes[] = { DeviceType::Vulkan, DeviceType::D3D11, DeviceType::D3D12, DeviceType::Metal };

    IDebugCallback* dbg           = AXM_NEW(SlangRHIDebugCallback);
    Unique<IDebugCallback> debugCallback(dbg);

    for (auto type: deviceTypes) {
        if (getRHI()->isDeviceTypeSupported(type)) {
            DeviceDesc deviceDesc                    = { };
            deviceDesc.deviceType                    = type;
            deviceDesc.debugCallback                 = debugCallback.get();
            deviceDesc.enableValidation              = true;

            Array<Feature, 2> requiredFeatures       = { Feature::Surface, Feature::Rasterization };
            deviceDesc.requiredFeatureCount          = static_cast<uint32_t>(requiredFeatures.size());
            deviceDesc.requiredFeatures              = requiredFeatures.data();
            deviceDesc.slang.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

            if (SLANG_SUCCEEDED(getRHI()->createDevice(deviceDesc, &device))) {
                AXM_LOG("Selected Rendering Backend : {}", getRHI()->getDeviceTypeName(type));
                break;
            }
        }
    }

    if (device == nullptr) {
        AXM_LOG("Failed to create slang-rhi device");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return { };
    }

    int width = 800, height = 600;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    ISurface* surface;
    if (SLANG_FAILED(device->createSurface(GetNativeWindowHandle(window), &surface))) {
        AXM_LOG("Failed to create surface from native window handle");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return { };
    }

    SurfaceConfig surfaceConfig = { };
    surfaceConfig.width         = width;
    surfaceConfig.height        = height;
    surfaceConfig.format        = Format::Undefined;
    surfaceConfig.vsync         = true;
    if (SLANG_FAILED(surface->configure(surfaceConfig))) {
        AXM_LOG("Failed to configure surface");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return { };
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    static ImFontConfig config { };
    config.FontDataOwnedByAtlas = false;

    ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*) &archivo_regular_ttf[0],
                                               sizeof(archivo_regular_ttf) / sizeof(archivo_regular_ttf[0]),
                                               14.0f,
                                               &config);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOther(window);
    if (!ImGui_ImplSlangRHI_Init(device, surface->getInfo().preferredFormat)) {
        AXM_LOG("Failed to initialize ImGui Slang RHI backend");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return { };
    }
    if (!Im3d_ImplSlangRHI_Init(device, surface->getInfo().preferredFormat, Format::D32Float)) {
        AXM_LOG("Failed to initialize Im3d Slang RHI backend");
        ImGui_ImplSlangRHI_Shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return { };
    }


    // Get command queue
    ICommandQueue* graphicsQueue;
    if (SLANG_FAILED(device->getQueue(QueueType::Graphics, &graphicsQueue))) {
        AXM_LOG("Failed to get Graphics queue");
        return { };
    }


    auto sampler = textures::CreateSampler(device, TextureFilteringMode::Linear, TextureAddressingMode::ClampToEdge);

    Array<String, 1> entries      = { "computeMain" };
    auto             mips         = Shader(device, "resources/shaders/mips", entries);
    auto             mipsPipeline = pipeline::CreateComputePipeline(device, mips);
    if (!mipsPipeline) {
        AXM_LOG_ERROR("Failed to create compute pipeline for generating mips.");
        return AxiomEngine::BAD();
    }

    DepthStencilDesc depthStencilDesc = { };
    depthStencilDesc.format           = Format::D32Float;
    depthStencilDesc.depthTestEnable  = true;
    depthStencilDesc.depthWriteEnable = true;
    depthStencilDesc.depthFunc        = ComparisonFunc::LessEqual;

    auto depthTexture                 = textures::CreateDepthTexture(device, width, height);

    // Fullscreen NanoVG Ctx
    NVGcontext* ctx             = nanovg::CreateContext(device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    bool        pipelineCreated = nanovg::CreatePipeline(ctx, surfaceConfig.format, depthStencilDesc.format);
    AXM_ASSERT(pipelineCreated, "Failed to create NanoVG Pipeline");

    GPU gpu = {
        .m_Device               = device,
        .m_Surface              = surface,
        .m_Queue                = graphicsQueue,
        .m_SwapchainColourImage = nullptr,
        .m_SwapchainDepthImage  = depthTexture,
        .m_DebugCallback        = std::move(debugCallback),
        .m_MipShader            = mips,
        .m_MipPipeline          = mipsPipeline,
        .m_LinearClampSampler   = sampler,
        .m_DepthStencilDesc     = depthStencilDesc,
        .m_FullScreenVG         = ctx,
    };

    Window w = { .m_Window = window, .m_Width = static_cast<u32>(width), .m_Height = static_cast<u32>(height) };

    return {
        .m_OK           = true,
        .m_Running      = true,
        .m_AssetManager = AssetManager(),
        .m_Window       = w,
        .m_GPU          = std::move(gpu),
    };
}

void axm::engine::Quit(const AxiomEngine& e) {
    PROFILE_SCOPE()

    e.m_GPU.m_Queue->waitOnHost();

    Im3d_ImplSlangRHI_Shutdown();
    ImGui_ImplSlangRHI_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(e.m_Window.m_Window);
    SDL_Quit();
}
bool axm::engine::PreFrame(AxiomEngine& e) {
    PROFILE_SCOPE()
    e.m_AssetManager.Update();
    e.m_FrameTimer.Reset();
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                e.m_Running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                OnWindowResized(e, event);
                break;
            default:
                // AXM_LOG("Unhandled event");
                break;
        }
    }

    ImGui_ImplSlangRHI_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (e.m_GPU.m_Surface->getConfig()) {
        e.m_GPU.m_Surface->acquireNextImage(&e.m_GPU.m_SwapchainColourImage);
        return true;
    }

    e.m_GPU.m_SwapchainColourImage = nullptr;
    return false;
}

void axm::engine::PostFrame(AxiomEngine& e) {
    PROFILE_SCOPE()

    ImGui::Render();

    if (e.m_GPU.m_SwapchainColourImage != nullptr) {
        auto commandEncoder = e.m_GPU.m_Queue->createCommandEncoder();
        nanovg::UpdateTextures(e.m_GPU.m_FullScreenVG, commandEncoder);

        auto passEncoder = render_pass::BeginSwapChainRenderPass(
                e, commandEncoder, rhi::LoadOp::Load, rhi::LoadOp::DontCare, false);

        nanovg::Render(e.m_GPU.m_FullScreenVG,
                       commandEncoder,
                       passEncoder,
                       static_cast<f32>(e.m_Window.m_Width),
                       static_cast<f32>(e.m_Window.m_Height));

        ImGui_ImplSlangRHI_RenderDrawData(ImGui::GetDrawData(), commandEncoder, passEncoder);

        passEncoder->end();
        e.m_GPU.m_Queue->submit(commandEncoder->finish());
        e.m_GPU.m_Surface->present();
    }

    e.m_DeltaTime = e.m_FrameTimer.ElapsedMillisecondsF();

    AXM_FLUSH_LOG();
}
void axm::engine::OnWindowResized(AxiomEngine& e, SDL_Event& ev) {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(e.m_Window.m_Window, &w, &h);

    if (e.m_GPU.m_Queue) {
        e.m_GPU.m_Queue->waitOnHost();
    }

    if (w > 0 && h > 0) {
        e.m_Window.m_Width               = static_cast<u32>(w);
        e.m_Window.m_Height              = static_cast<u32>(h);

        rhi::SurfaceConfig surfaceConfig = { };
        surfaceConfig.width              = static_cast<uint32_t>(w);
        surfaceConfig.height             = static_cast<uint32_t>(h);
        surfaceConfig.format             = rhi::Format::Undefined;
        surfaceConfig.vsync              = true;

        if (SLANG_FAILED(e.m_GPU.m_Surface->configure(surfaceConfig))) {
            AXM_LOG_ERROR("Failed to reconfigure surface on resize");
        }

        e.m_GPU.m_SwapchainDepthImage = textures::CreateDepthTexture(e.m_GPU.m_Device, w, h);
    } else {
        e.m_GPU.m_Surface->unconfigure();
    }
}

axm::AxiomEngine axm::AxiomEngine::BAD() {
    PROFILE_SCOPE()

    return { .m_OK = false, .m_Running = false, .m_AssetManager = AssetManager(), .m_Window = nullptr, .m_GPU = GPU() };
}
