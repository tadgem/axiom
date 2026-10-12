# Axiom

Axiom is a C++23 game engine and rendering framework built on Vulkan, featuring ECS, physics, scripting, and a full rendering pipeline.

## Features

- **Vulkan Rendering** — Modern graphics powered by the [vku](https://github.com/tadgem/vku) helper library, with deferred and forward rendering examples
- **Entity Component System** — [Flecs](https://github.com/SanderMertens/flecs) ECS integration
- **Physics** — [Jolt Physics](https://github.com/jrouwe/JoltPhysics) integration
- **Scripting** — Lua 5.1 (via LuaJIT) with an `axiom.*` API
- **Assets** — glTF/ASSIMP model loading, texture management, JSON persistence
- **UI** — ImGui + ImGuizmo for immediate-mode editor tools, NanoVG for custom UI
- **Maths** — Custom linear algebra (`aml`) with matrix, vector, and quaternion support
- **Profiling** — Integrated profiler with ImGui visualization
- **Input & Windows** — SDL3-based windowing and input handling
- **Editor Tools** — Transform gizmos, fly camera, asset browser, viewport

## Architecture

```
axiom/
├── include/              # Public headers
│   ├── Core/             # Engine core (Window, Input, Timer, Maths, JSON, STL)
│   ├── Render/           # Camera, Mesh, Pipeline, Shader, Transform, Viewport
│   ├── Assets/           # AssetManager, Model, Texture, Defaults
│   └── Script/           # Lua bindings
├── src/                  # Engine implementation
├── examples/             # Render pipeline examples (deferred, forward)
├── lab/                  # Experimentation / sandbox
├── resources/            # Shared assets (models, shaders, textures)
├── tests/                # Unit tests
├── third_party/          # Vendored dependencies
└── submodules/           # Git submodule dependencies
```

### Key Components

| Component | Description |
|-----------|-------------|
| `AxiomEngine` | Core engine entry point. `AxiomEngine::Init()` returns the engine instance; `PreFrame()`/`PostFrame()` manage the main loop. |
| `AssetManager` | Loads and manages assets (models, textures, etc.) with handle-based referencing. |
| `vku::VkState` | Vulkan state wrapper from the vku library (device, swapchain, command buffers, render passes). |
| `flecs::world` | ECS world for entity-component systems. |
| `JoltPhysics` | Physics world integration. |
| `Lua` | Embedded LuaJIT interpreter accessible via the `axiom` global table. |

## Dependencies

### Submodules
- [**flecs**](https://github.com/SanderMertens/flecs) — ECS framework
- [**Jolt Physics**](https://github.com/jrouwe/JoltPhysics) — Physics engine
- [**vku**](https://github.com/tadgem/vku) — Vulkan helper library (provides SDL3, ASSIMP, GLM, volk, ImGui, NanoVG, im3d, shaderc)
- [**luajit-cmake**](https://github.com/tadgem/luajit-cmake) — LuaJIT CMake build integration

### Third-Party
| Library | Version | Purpose |
|---------|---------|---------|
| mimalloc | 3.3.2 | Memory allocator |
| nlohmann_json | 3.12.0 | JSON parsing |
| ImGuizmo | 1.10 | Transform gizmos |
| stb_image | 2.30 | Image loading |
| GLM | — | Mathematics |
| volk | — | Vulkan loader |

## Build Requirements

- **OS**: Windows (MSVC) / Linux
- **CMake**: 3.18+
- **C++ Compiler**: C++23 compliant (MSVC 19.33+ recommended)
- **Vulkan SDK**: 1.3+ with development headers
- **SDL3**: Provided by the vku submodule
- **LuaJIT**: Built via submodule

## Building

### 1. Clone with submodules

```bash
git clone --recurse-submodules https://github.com/tadgem/axiom.git
cd axiom
```

Or if you already cloned:

```bash
git submodule update --init --recursive
```

### 2. Configure with CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### 3. Build

```bash
cmake --build build --config Debug
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `AXIOM_BUILD_TESTS` | `ON` (top-level) | Build unit tests |
| `AXIOM_BUILD_EXAMPLES` | `ON` (top-level) | Build example applications |

## Running Examples

After building, executables are located in the build directory.

```bash
# Copy resources if needed (handled by CMake pre-build step)
# Run the deferred rendering example
./build/Debug/axiom-example-deferred.exe

# Run the forward rendering example
./build/Debug/axiom-example-forward.exe

# Run the lab/experiments
./build/Debug/axiom-lab.exe
```

The examples render the **Sponza scene** (`resources/models/sponza/Sponza.gltf`) with either a deferred or forward pipeline. Use WASD + mouse to fly around, and the ImGui panel provides transform/camera editors.

## Testing

```bash
# From the build directory
ctest --build-config Debug --verbose
```

## Project Layout

```
CMakeLists.txt          # Top-level CMake configuration
CMakeLists.txt            axm:: namespace for all axiom types
axiom/                  # Main library target ("axiom")
  include/              # Public header files
  src/                  # Source files
examples/               # Standalone example executables
  deferred/             # Deferred rendering example
  forward/              # Forward rendering example
lab/                    # Sandbox / experiments
tests/                  # Unit tests
resources/              # Shared assets (models, shaders, textures)
third_party/            # Vendored single-header libraries
submodules/             # Git submodule dependencies
cmake/                  # CMake utility functions
meta/                   # Project documentation and drawings
```

## Quick Start

```cpp
#include "axiom.hpp"

using namespace axm;

int main() {
    // Initialize the engine
    AxiomEngine engine = AxiomEngine::Init();
    AXM_ASSERT(engine.m_OK, "Failed to start Axiom");

    while (engine.PreFrame()) {
        // Your game logic here
        // Access Vulkan via engine.m_VK
        // Access input via engine.m_Input
        // Access assets via engine.m_AssetManager
    }

    engine.Quit();
    return 0;
}
```

## License

This project is released into the **public domain** under the [Unlicense](LICENSE). Anyone is free to use, modify, publish, and distribute this software for any purpose.

## Acknowledgments

- **vku** by tadgem — Vulkan abstraction layer
- **Flecs** by Sander Mertens — ECS framework
- **Jolt Physics** by Jorren Romkes — Physics engine
- **LuaJIT** by Mike Pall — High-performance Lua interpreter
- **ASSIMP** by Assimp project — Asset import library
- **ImGuizmo** by Olivier THOUvard — Transform gizmos for ImGui
