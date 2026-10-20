#include "Core/Engine.hpp"

#include <cstring>

#include "ArchivoRegularTTF.h"
#include "Assets/Defaults.hpp"
#include "Core/Debug.hpp"
#include "Core/Profile.hpp"
#include "Core/Utils.hpp"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include "SDL3/SDL.h"
#include "VkSDL.h"
#include "nanovg.h"
#include "vfspp/VFS.h"
#include "vku/Init.h"
#include "vku/Texture.h"

axm::AxiomEngine::AxiomEngine() : m_FileSystem(MakeUnique<vfspp::VirtualFileSystem>()), m_AssetManager(*m_FileSystem) {}

axm::AxiomEngine axm::AxiomEngine::Init()
{
	PROFILE_SCOPE()

	AxiomEngine  engine     = {};

	vku::VkState vk         = vku::init::Create<vku::VkSDL>("AXIOM", 1280, 720, false, true);

	engine.m_GPU            = MakeUnique<GPU>(vk);

	const uint32_t MAX_MIPS = 16;

	vku::textures::CreateImageSampler(engine.m_GPU->m_VK,
	                                  MAX_MIPS,
	                                  VK_FILTER_LINEAR,
	                                  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	                                  engine.m_GPU->m_Samplers.m_LinearClampSampler);
	vku::textures::CreateImageSampler(engine.m_GPU->m_VK,
	                                  MAX_MIPS,
	                                  VK_FILTER_LINEAR,
	                                  VK_SAMPLER_ADDRESS_MODE_REPEAT,
	                                  engine.m_GPU->m_Samplers.m_LinearWrapSampler);
	vku::textures::CreateImageSampler(engine.m_GPU->m_VK,
	                                  MAX_MIPS,
	                                  VK_FILTER_NEAREST,
	                                  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	                                  engine.m_GPU->m_Samplers.m_NearestClampSampler);
	vku::textures::CreateImageSampler(engine.m_GPU->m_VK,
	                                  MAX_MIPS,
	                                  VK_FILTER_NEAREST,
	                                  VK_SAMPLER_ADDRESS_MODE_REPEAT,
	                                  engine.m_GPU->m_Samplers.m_NearestWrapSampler);

	auto* windowHandle = static_cast<vku::VulkanAPIWindowHandle_SDL*>(engine.m_GPU->m_VK.m_WindowHandle);
	if (!windowHandle || !windowHandle->m_SdlWindow)
	{
		AXM_LOG_ERROR("Failed to obtain SDL window handle from vku backend");
		return AxiomEngine::BAD();
	}
	engine.m_Window.m_Window = windowHandle->m_SdlWindow;
	int width = 0, height = 0;
	SDL_GetWindowSizeInPixels(engine.m_Window.m_Window, &width, &height);
	engine.m_Window.m_Width  = CAST(width, u32);
	engine.m_Window.m_Height = CAST(height, u32);

	// TODO (LiamD) : Move this to somewhere more appropriate.
	ImFontConfig cfg;
	cfg.FontDataOwnedByAtlas   = false;
	ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
	        &archivo_regular_ttf[0], _countof(archivo_regular_ttf), 14.0f, &cfg);
	ImGui_ImplVulkan_CreateFontsTexture();

	engine.m_OK      = true;
	engine.m_Running = engine.m_GPU->m_VK.m_ShouldRun;

	if (!engine.m_FileSystem->CreateFileSystem<vfspp::NativeFileSystem>("/", "resources"))
	{
		AXM_LOG_ERROR("Failed to initalize virtual file system");
		return AxiomEngine::BAD();
	}
	AddDefaultAssetHandlers(engine);

	AXM_FLUSH_LOG();
	return engine;
}

axm::AxiomEngine axm::AxiomEngine::BAD()
{
	Unique<vfspp::VirtualFileSystem> vfs = MakeUnique<vfspp::VirtualFileSystem>();
	AxiomEngine                      engine;
	engine.m_OK      = false;
	engine.m_Running = false;
	return engine;
}

axm::AxiomEngine::~AxiomEngine()
{
	if (m_GPU)
	{
		Quit();
	}
}

void axm::AxiomEngine::Quit()
{
	PROFILE_SCOPE()

	m_AssetManager.UnloadAllAssets();

	if (m_GPU)
	{
		m_GPU->m_Samplers.Free(m_GPU->m_VK);
		vku::init::Quit(m_GPU->m_VK);
		m_GPU.reset();
	}
}

bool axm::AxiomEngine::PreFrame()
{
	PROFILE_SCOPE()

	m_AssetManager.Update();
	m_FrameTimer.Reset();
	m_Input.ClearInputs();

	auto* sdlBackend      = static_cast<vku::VkSDL*>(m_GPU->m_VK.m_Backend.get());
	sdlBackend->m_OnEvent = [this](SDL_Event& event) { m_Input.HandleFrameInputEvent(event); };

	// Polls events (feeding the input hook above), rebuilds the ImGui frame and
	// waits for/acquires the next swapchain image.
	m_GPU->m_VK.m_Backend->PreFrame(m_GPU->m_VK);

	sdlBackend->m_OnEvent = nullptr;

	ImGuizmo::BeginFrame();

	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(m_Window.m_Window, &w, &h);
	if (w > 0 && h > 0)
	{
		m_Window.m_Width  = CAST(w, u32);
		m_Window.m_Height = CAST(h, u32);
	}

	m_Running = m_GPU->m_VK.m_ShouldRun;

	return m_GPU->m_VK.m_FrameValid;
}

void axm::AxiomEngine::PostFrame()
{
	PROFILE_SCOPE()

	m_GPU->m_VK.m_Backend->PostFrame(m_GPU->m_VK);

	m_DeltaTime = m_FrameTimer.ElapsedMillisecondsF();

	AXM_FLUSH_LOG();
}

void axm::AxiomEngine::OnWindowResized(SDL_Event& ev)
{
	(void) ev;
	if (m_Window.m_Window == nullptr)
	{
		return;
	}

	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(m_Window.m_Window, &w, &h);
	if (w > 0 && h > 0)
	{
		m_Window.m_Width  = CAST(w, u32);
		m_Window.m_Height = CAST(h, u32);
	}
}
