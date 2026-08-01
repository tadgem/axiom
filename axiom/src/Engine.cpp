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

axm::AxiomEngine axm::AxiomEngine::Init() {
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
            deviceDesc.requiredFeatureCount          = CAST(requiredFeatures.size(), uint32_t);
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

    i32 width = 800, height = 600;
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


    auto linearClampSampler
            = textures::CreateSampler(device, TextureFilteringMode::Linear, TextureAddressingMode::ClampToEdge);
    auto linearWrapSampler = textures::CreateSampler(device, TextureFilteringMode::Linear, TextureAddressingMode::Wrap);

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
    NVGcontext* ctx      = nanovg::CreateContext(device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    bool pipelineCreated = nanovg::CreatePipeline(ctx, surface->getInfo().preferredFormat, rhi::Format::Undefined);
    AXM_ASSERT(pipelineCreated, "Failed to create NanoVG Pipeline");

    // create default font in fullscreen context.
    i32 fontSans = nvgCreateFontMem(ctx, "sans", (unsigned char*) archivo_regular_ttf, sizeof(archivo_regular_ttf), 0);
    AXM_ASSERT(fontSans != -1, "Failed to load default font into NanoVG");

    rhi::ITexture* swapChainTexture = nullptr;
    if (surface->getConfig()) {
        surface->acquireNextImage(&swapChainTexture);
    }

    GPU gpu = {
        .m_Device               = device,
        .m_Surface              = surface,
        .m_Queue                = graphicsQueue,
        .m_SwapchainColourImage = swapChainTexture,
        .m_SwapchainDepthImage  = depthTexture,
        .m_DebugCallback        = std::move(debugCallback),
        .m_MipShader            = mips,
        .m_MipPipeline          = mipsPipeline,
        .m_LinearClampSampler   = linearClampSampler,
        .m_LinearWrapSampler    = linearWrapSampler,
        .m_DepthStencilDesc     = depthStencilDesc,
        .m_FullScreenVG         = ctx,
    };

    Window w = { .m_Window = window, .m_Width = CAST(width, u32), .m_Height = CAST(height, u32) };

    return {
        .m_OK           = true,
        .m_Running      = true,
        .m_AssetManager = AssetManager(),
        .m_Window       = w,
        .m_GPU          = std::move(gpu),
    };
}

void axm::AxiomEngine::Quit() const {
    PROFILE_SCOPE()

    m_GPU.m_Queue->waitOnHost();

    Im3d_ImplSlangRHI_Shutdown();
    ImGui_ImplSlangRHI_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(m_Window.m_Window);
    SDL_Quit();
}

bool axm::AxiomEngine::PreFrame() {
    PROFILE_SCOPE()
    m_AssetManager.Update();
    m_FrameTimer.Reset();
    {
        NAMED_SCOPE(FrameInputHandling)
        m_Input.ClearInputs();

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            m_Input.HandleFrameInputEvent(event);
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    m_Running = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    OnWindowResized(event);
                    break;
                default:
                    // AXM_LOG("Unhandled event");
                    break;
            }
        }
    }
    ImGui_ImplSlangRHI_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (m_GPU.m_Surface->getConfig()) {
        m_GPU.m_Surface->acquireNextImage(&m_GPU.m_SwapchainColourImage);
        return true;
    }

    m_GPU.m_SwapchainColourImage = nullptr;
    return false;
}

void axm::AxiomEngine::PostFrame() {
    PROFILE_SCOPE()

    ImGui::Render();

    if (m_GPU.m_SwapchainColourImage != nullptr) {
        auto commandEncoder = m_GPU.m_Queue->createCommandEncoder();
        nanovg::UpdateTextures(m_GPU.m_FullScreenVG, commandEncoder);

        auto passEncoder = render_pass::BeginSwapChainRenderPass(
                m_GPU, commandEncoder, rhi::LoadOp::Load, rhi::LoadOp::DontCare, false);

        nanovg::Render(m_GPU.m_FullScreenVG,
                       commandEncoder,
                       passEncoder,
                       CAST(m_Window.m_Width, f32),
                       CAST(m_Window.m_Height, f32));

        ImGui_ImplSlangRHI_RenderDrawData(ImGui::GetDrawData(), commandEncoder, passEncoder);

        passEncoder->end();
        m_GPU.m_Queue->submit(commandEncoder->finish());
        m_GPU.m_Surface->present();
    }

    m_DeltaTime = m_FrameTimer.ElapsedMillisecondsF();

    AXM_FLUSH_LOG();
}
void axm::AxiomEngine::OnWindowResized(SDL_Event& ev) {
    i32 w = 0, h = 0;
    SDL_GetWindowSizeInPixels(m_Window.m_Window, &w, &h);

    if (m_GPU.m_Queue) {
        m_GPU.m_Queue->waitOnHost();
    }

    if (w > 0 && h > 0) {
        m_Window.m_Width                 = CAST(w, u32);
        m_Window.m_Height                = CAST(h, u32);

        rhi::SurfaceConfig surfaceConfig = { };
        surfaceConfig.width              = CAST(w, uint32_t);
        surfaceConfig.height             = CAST(h, uint32_t);
        surfaceConfig.format             = rhi::Format::Undefined;
        surfaceConfig.vsync              = true;

        if (SLANG_FAILED(m_GPU.m_Surface->configure(surfaceConfig))) {
            AXM_LOG_ERROR("Failed to reconfigure surface on resize");
        }

        m_GPU.m_SwapchainDepthImage = textures::CreateDepthTexture(m_GPU.m_Device, w, h);
    } else {
        m_GPU.m_Surface->unconfigure();
    }
}

axm::AxiomEngine axm::AxiomEngine::BAD() {
    PROFILE_SCOPE()

    return { .m_OK = false, .m_Running = false, .m_AssetManager = AssetManager(), .m_Window = nullptr, .m_GPU = GPU() };
}
