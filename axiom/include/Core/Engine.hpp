#pragma once
#include "Assets/AssetManager.hpp"
#include "Core/Input.hpp"
#include "Core/STL.hpp"
#include "Core/Timer.hpp"
#include "Core/Window.hpp"
#include "Render/GPU.hpp"
#include "SDL3/SDL.h"
#include "vku/vku.h"

namespace axm {
    struct AxiomEngine
    {
        bool                            m_OK      = false;
        bool                            m_Running = true;
        Timer                           m_FrameTimer;
        f64                             m_DeltaTime = 0.0;

        AssetManager                    m_AssetManager;
        Window                          m_Window;
        GPU                             m_GPU;
        Input                           m_Input;

        Unique<vku::VkState>            m_VK;

        static AxiomEngine              BAD();
        static AxiomEngine              Init();

        ~AxiomEngine();

        // Move-only: owns the vku::VkState (and thus the Vulkan device).
        AxiomEngine() = default;
        AxiomEngine(const AxiomEngine&) = delete;
        AxiomEngine& operator=(const AxiomEngine&) = delete;
        AxiomEngine(AxiomEngine&&) noexcept = default;
        AxiomEngine& operator=(AxiomEngine&&) noexcept = default;

        void                            Quit();
        NO_DISCARD bool                 PreFrame();
        void                            PostFrame();
        void                            OnWindowResized(SDL_Event& ev);
    };

} // namespace axm
