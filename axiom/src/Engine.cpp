#include "Core/Engine.hpp"
#include "ArchivoRegularTTF.h"
#include "Core/Debug.hpp"
#include "Core/Profile.hpp"
#include "Core/Utils.hpp"

#include "VkSDL.h"
#include "vku/Init.h"
#include "vku/Texture.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include "nanovg.h"

#include "SDL3/SDL.h"

#include <cstring>

axm::AxiomEngine axm::AxiomEngine::Init() {
    PROFILE_SCOPE()

    AxiomEngine engine;

    vku::VkState vk = vku::init::Create<vku::VkSDL>("AXIOM", 1280, 720, false, true);
    engine.m_VK     = MakeUnique<vku::VkState>(std::move(vk));

    engine.m_GPU.m_VK           = engine.m_VK.get();
    

    vku::textures::CreateImageSampler(*engine.m_VK,
                                      1,
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                      engine.m_GPU.m_LinearClampSampler);
    vku::textures::CreateImageSampler(*engine.m_VK,
                                      1,
                                      VK_FILTER_LINEAR,
                                      VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                      engine.m_GPU.m_LinearWrapSampler);

    auto* windowHandle = static_cast<vku::VulkanAPIWindowHandle_SDL*>(engine.m_VK->m_WindowHandle);
    if (!windowHandle || !windowHandle->m_SdlWindow) {
        AXM_LOG_ERROR("Failed to obtain SDL window handle from vku backend");
        return AxiomEngine::BAD();
    }
    engine.m_Window.m_Window = windowHandle->m_SdlWindow;
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(engine.m_Window.m_Window, &width, &height);
    engine.m_Window.m_Width  = CAST(width, u32);
    engine.m_Window.m_Height = CAST(height, u32);

    // Default font for both ImGui and NanoVG.
    static ImFontConfig config{};
    config.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)&archivo_regular_ttf[0],
                                               CAST(sizeof(archivo_regular_ttf), int),
                                               14.0f,
                                               &config);
    ImGui_ImplVulkan_CreateFontsTexture();

    // NOTE: vku's Vulkan NanoVG backend does not initialise a font stash, so
    // font registration (nvgCreateFontMem) is not available. NanoVG is used for
    // shape/vector rendering only.

    engine.m_OK      = true;
    engine.m_Running = engine.m_VK->m_ShouldRun;

    AXM_FLUSH_LOG();
    return engine;
}

axm::AxiomEngine axm::AxiomEngine::BAD() {
    AxiomEngine engine;
    engine.m_OK      = false;
    engine.m_Running = false;
    return engine;
}

axm::AxiomEngine::~AxiomEngine() {
    if (m_VK) {
        Quit();
    }
}

void axm::AxiomEngine::Quit() {
    PROFILE_SCOPE()
    m_AssetManager.UnloadAllAssets();
    if (m_VK) {
        vku::init::Quit(*m_VK);
        m_VK.reset();
    }
}

bool axm::AxiomEngine::PreFrame() {
    PROFILE_SCOPE()

    m_AssetManager.Update();
    m_FrameTimer.Reset();
    m_Input.ClearInputs();

    auto* sdlBackend = static_cast<vku::VkSDL*>(m_VK->m_Backend.get());
    sdlBackend->m_OnEvent = [this](SDL_Event& event) { m_Input.HandleFrameInputEvent(event); };

    // Polls events (feeding the input hook above), rebuilds the ImGui frame and
    // waits for/acquires the next swapchain image.
    m_VK->m_Backend->PreFrame(*m_VK);

    sdlBackend->m_OnEvent = nullptr;

    ImGuizmo::BeginFrame();

    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(m_Window.m_Window, &w, &h);
    if (w > 0 && h > 0) {
        m_Window.m_Width  = CAST(w, u32);
        m_Window.m_Height = CAST(h, u32);
    }

    m_Running = m_VK->m_ShouldRun;

    return m_VK->m_FrameValid;
}

void axm::AxiomEngine::PostFrame() {
    PROFILE_SCOPE()

    m_VK->m_Backend->PostFrame(*m_VK);

    m_DeltaTime = m_FrameTimer.ElapsedMillisecondsF();

    AXM_FLUSH_LOG();
}

void axm::AxiomEngine::OnWindowResized(SDL_Event& ev) {
    (void)ev;
    if (m_Window.m_Window == nullptr) {
        return;
    }

    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(m_Window.m_Window, &w, &h);
    if (w > 0 && h > 0) {
        m_Window.m_Width  = CAST(w, u32);
        m_Window.m_Height = CAST(h, u32);
    }
}
