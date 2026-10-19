#pragma once
#include "Assets/AssetManager.hpp"
#include "Core/Input.hpp"
#include "Core/STL.hpp"
#include "Core/Timer.hpp"
#include "Core/Window.hpp"
#include "ECS/ECS.hpp"
#include "Render/GPU.hpp"
#include "SDL3/SDL.h"
#include "vfspp/VFS.h"
namespace axm
{
struct AxiomEngine
{
	bool                             m_OK      = false;
	bool                             m_Running = true;
	Timer                            m_FrameTimer;
	f64                              m_DeltaTime = 0.0;

	Unique<vfspp::VirtualFileSystem> m_FileSystem;
	AssetManager                     m_AssetManager;
	ECSDispatcher                    m_ECS;
	Window                           m_Window;
	Unique<GPU>                      m_GPU;
	Input                            m_Input;

	bool                             m_Quit = false;

	static AxiomEngine               BAD();
	static AxiomEngine               Init();

	~AxiomEngine();

	// Move-only: owns the vku::VkState (and thus the Vulkan device).
	AxiomEngine();
	AxiomEngine(const AxiomEngine&)                   = delete;
	AxiomEngine& operator=(const AxiomEngine&)        = delete;
	AxiomEngine(AxiomEngine&&) noexcept               = default;
	AxiomEngine&    operator=(AxiomEngine&&) noexcept = default;

	void            Quit();
	NO_DISCARD bool PreFrame();
	void            PostFrame();
	void            OnWindowResized(SDL_Event& ev);
};

}  // namespace axm
