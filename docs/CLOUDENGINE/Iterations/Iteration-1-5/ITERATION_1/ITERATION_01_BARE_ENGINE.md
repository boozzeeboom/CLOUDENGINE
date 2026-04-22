# Iteration 1: Bare Engine Foundation
## CLOUDENGINE — Minimal Viable Engine

**Duration:** 1-2 weeks  
**Goal:** Working window, input, ECS, game loop  
**Deliverable:** Console window with "Hello World" in OpenGL

---

## 1. What to Download

### Required Libraries (all header-only or single-file):

| Library | Version | Download | Size | Purpose |
|---------|---------|----------|------|---------|
| **GLFW** | 3.4 | https://github.com/glfw/glfw/releases/tag/3.4 | ~5MB | Window, Input |
| **GLM** | 1.0.0 | https://github.com/g-truc/glm/releases/tag/1.0.0 | ~2MB | Math |
| **flecs** | 3.2.x | https://github.com/SanderMertens/flecs/archive/refs/tags/v3.2.5.zip | ~3MB | ECS |
| **spdlog** | 1.12 | https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.zip | ~0.5MB | Logging |

### Build Tools:

| Tool | Version | Download | Purpose |
|------|---------|----------|---------|
| **CMake** | 3.28+ | https://cmake.org/download/ | Build system |
| **MinGW-w64** or **MSVC** | Latest | VS Installer or https://www.mingw-w64.org/ | C++ Compiler |

---

## 2. Directory Structure

```
CLOUDENGINE/
├── src/
│   ├── core/
│   │   ├── engine.h
│   │   ├── engine.cpp
│   │   ├── logging.h
│   │   └── logging.cpp
│   ├── platform/
│   │   ├── window.h
│   │   └── window.cpp
│   ├── ecs/
│   │   ├── world.h
│   │   └── world.cpp
│   ├── rendering/
│   │   ├── renderer.h
│   │   └── renderer.cpp
│   └── main.cpp
├── libs/
│   ├── glfw/
│   ├── glm/
│   ├── flecs/
│   └── spdlog/
├── build/
├── CMakeLists.txt
└── README.md
```

---

## 3. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(CLOUDENGINE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/libs/glfw/include
    ${CMAKE_SOURCE_DIR}/libs/glm
    ${CMAKE_SOURCE_DIR}/libs/flecs
    ${CMAKE_SOURCE_DIR}/libs/spdlog/include
    ${CMAKE_SOURCE_DIR}/src
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Source files
file(GLOB SOURCES
    "src/core/*.cpp"
    "src/platform/*.cpp"
    "src/ecs/*.cpp"
    "src/rendering/*.cpp"
)

# Executable
add_executable(CloudEngine ${SOURCES} src/main.cpp)

# Link libraries
target_link_libraries(CloudEngine
    ${OPENGL_gl_LIBRARY}
    glfw3
    pthread
)

# GLFW build (static)
add_subdirectory(libs/glfw)
target_link_libraries(CloudEngine glfw)
```

---

## 4. Source Files

### src/core/logging.h

```cpp
#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace Core {

class Logger {
public:
    static void init(const char* filename = "cloudengine.log") {
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);
        spdlog::info("CLOUDENGINE initializing...");
    }
    
    static void shutdown() {
        spdlog::drop_all();
    }
};

} // namespace Core

#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_DEBUG(...) spdlog::debug(__VA_ARGS__)
```

### src/core/engine.h

```cpp
#pragma once
#include <cstdint>

namespace Core {

class Engine {
public:
    Engine();
    ~Engine();
    
    bool init();
    void shutdown();
    void run();
    
    float getDeltaTime() const { return _deltaTime; }
    float getTime() const { return _time; }
    
private:
    void update(float dt);
    void render();
    
    bool _running = false;
    float _deltaTime = 0.016f;
    float _time = 0.0f;
    uint64_t _lastTime = 0;
};

} // namespace Core
```

### src/core/engine.cpp

```cpp
#include "engine.h"
#include "logging.h"
#include <platform/window.h>
#include <ecs/world.h>
#include <rendering/renderer.h>

namespace Core {

Engine::Engine() {}
Engine::~Engine() {}

bool Engine::init() {
    LOG_INFO("Initializing Engine...");
    
    if (!Platform::Window::init(1280, 720, "Project C: The Clouds")) {
        LOG_ERROR("Failed to initialize window");
        return false;
    }
    LOG_INFO("Window initialized");
    
    if (!Rendering::Renderer::init()) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }
    LOG_INFO("Renderer initialized");
    
    ECS::World::init();
    LOG_INFO("ECS initialized");
    
    _running = true;
    LOG_INFO("Engine initialized successfully");
    return true;
}

void Engine::shutdown() {
    LOG_INFO("Shutting down Engine...");
    ECS::World::shutdown();
    Rendering::Renderer::shutdown();
    Platform::Window::shutdown();
    Logger::shutdown();
    LOG_INFO("Engine shutdown complete");
}

void Engine::run() {
    LOG_INFO("Engine running...");
    _lastTime = getCurrentTimeMs();
    
    while (_running && !Platform::Window::shouldClose()) {
        uint64_t currentTime = getCurrentTimeMs();
        float dt = (currentTime - _lastTime) / 1000.0f;
        _lastTime = currentTime;
        
        update(dt);
        render();
        
        Platform::Window::pollEvents();
    }
    
    shutdown();
}

void Engine::update(float dt) {
    _time += dt;
    _deltaTime = dt;
    
    ECS::World::update(dt);
    
    // Exit on Escape
    if (Platform::Window::isKeyPressed(GLFW_KEY_ESCAPE)) {
        _running = false;
    }
}

void Engine::render() {
    Rendering::Renderer::beginFrame();
    Rendering::Renderer::clear(0.4f, 0.6f, 0.9f, 1.0f); // Sky blue
    Rendering::Renderer::endFrame();
}

} // namespace Core
```

### src/platform/window.h

```cpp
#pragma once
#include <GLFW/glfw3.h>

namespace Core { namespace Platform {

class Window {
public:
    static bool init(int width, int height, const char* title);
    static void shutdown();
    static bool shouldClose();
    static void pollEvents();
    static bool isKeyPressed(int key);
    static void setTitle(const char* title);
    static int getWidth() { return _width; }
    static int getHeight() { return _height; }
    
private:
    static GLFWwindow* _window;
    static int _width;
    static int _height;
};

}} // namespace Core::Platform
```

### src/platform/window.cpp

```cpp
#include "window.h"
#include "../core/logging.h"
#include <GLFW/glfw3.h>

namespace Core { namespace Platform {

GLFWwindow* Window::_window = nullptr;
int Window::_width = 0;
int Window::_height = 0;

bool Window::init(int width, int height, const char* title) {
    _width = width;
    _height = height;
    
    if (!glfwInit()) {
        LOG_ERROR("GLFW init failed");
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window) {
        LOG_ERROR("Window creation failed");
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(_window);
    LOG_INFO("GLFW window created: {}x{}", width, height);
    return true;
}

void Window::shutdown() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
    LOG_INFO("GLFW shutdown");
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

bool Window::isKeyPressed(int key) {
    return glfwGetKey(_window, key) == GLFW_PRESS;
}

void Window::setTitle(const char* title) {
    if (_window) {
        glfwSetWindowTitle(_window, title);
    }
}

}} // namespace Core::Platform
```

### src/ecs/world.h

```cpp
#pragma once
#include <flecs.h>

namespace Core { namespace ECS {

class World {
public:
    static void init();
    static void shutdown();
    static void update(float dt);
    static flecs::world& get() { return _world; }
    
private:
    static flecs::world _world;
};

}} // namespace Core::ECS
```

### src/ecs/world.cpp

```cpp
#include "world.h"
#include "../core/logging.h"

namespace Core { namespace ECS {

flecs::world World::_world;

void World::init() {
    _world = flecs::world();
    _world.set_target_fps(60);
    
    // Example system: Position + Velocity
    _world.system<struct Position, struct Velocity>("Movement")
        .kind(flecs::OnUpdate)
        .each([](struct Position& p, struct Velocity& v) {
            p.x += v.x * 0.016f;
            p.y += v.y * 0.016f;
            p.z += v.z * 0.016f;
        });
    
    LOG_INFO("ECS World created");
}

void World::shutdown() {
    _world = flecs::world();
    LOG_INFO("ECS World destroyed");
}

void World::update(float dt) {
    _world.progress(dt);
}

}} // namespace Core::ECS

// Components
struct Position { float x = 0, y = 0, z = 0; };
struct Velocity { float x = 0, y = 0, z = 0; };
```

### src/rendering/renderer.h

```cpp
#pragma once
#include <cstdint>

namespace Core { namespace Rendering {

class Renderer {
public:
    static bool init();
    static void shutdown();
    static void beginFrame();
    static void endFrame();
    static void clear(float r, float g, float b, float a);
    
private:
    static bool _initialized;
};

}} // namespace Core::Rendering
```

### src/rendering/renderer.cpp

```cpp
#include "renderer.h"
#include <GL/gl.h>
#include "../core/logging.h"

namespace Core { namespace Rendering {

bool Renderer::_initialized = false;

bool Renderer::init() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f);
    _initialized = true;
    LOG_INFO("OpenGL renderer initialized");
    return true;
}

void Renderer::shutdown() {
    _initialized = false;
    LOG_INFO("Renderer shutdown");
}

void Renderer::beginFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    // Swap buffers handled by Window
}

void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}} // namespace Core::Rendering
```

### src/main.cpp

```cpp
#include <core/logging.h>
#include <core/engine.h>
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace Core;
    
    // Init logging first
    Logger::init();
    LOG_INFO("=== CLOUDENGINE v0.1 ===");
    LOG_INFO("Project C: The Clouds - Minimal Engine");
    
    // Create and run engine
    Engine engine;
    if (!engine.init()) {
        LOG_ERROR("Engine failed to initialize!");
        return 1;
    }
    
    engine.run();
    
    LOG_INFO("Exiting normally");
    return 0;
}
```

---

## 5. Build Instructions (Windows)

### Step 1: Download Libraries

```bash
# Create libs directory
mkdir libs

# Download GLFW
curl -L https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip -o libs/glfw.zip
unzip libs/glfw.zip -d libs/glfw/

# Download GLM
curl -L https://github.com/g-truc/glm/releases/download/1.0.0/glm-1.0.0.7z -o libs/glm.7z
7z x libs/glm.7z -olibs/glm

# Download flecs
curl -L https://github.com/SanderMertens/flecs/archive/refs/tags/v3.2.5.zip -o libs/flecs.zip
unzip libs/flecs.zip -d libs/flecs

# Download spdlog
curl -L https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.zip -o libs/spdlog.zip
unzip libs/spdlog.zip -d libs/spdlog
```

### Step 2: Create Build Directory

```bash
mkdir build
cd build
```

### Step 3: Configure with CMake

```bash
cmake .. -G "MinGW Makefiles"  # or "Visual Studio 17 2022"
```

### Step 4: Build

```bash
cmake --build . --config Release
```

### Step 5: Run

```bash
./CloudEngine.exe
```

Expected output: A window with sky blue background that closes on ESC.

---

## 6. Verification Checklist

| Check | Expected Result |
|-------|-----------------|
| [ ] Window opens | 1280x720 window titled "Project C: The Clouds" |
| [ ] Console log | "CLOUDENGINE initializing..." etc. |
| [ ] ESC exits | Window closes on Escape press |
| [ ] No crashes | Clean shutdown |

---

## 7. Common Issues

### "glfw3.h not found"
- Ensure GLFW is in `libs/glfw/include/GLFW/glfw3.h`

### "unresolved external glew"
- Add GLEW: `find_package(GLEW REQUIRED)`

### CMake not finding OpenGL
- Install MinGW-w64 with OpenGL support
- Or use Vcpkg: `vcpkg install glfw3`

---

## 8. Next Steps

After this iteration:
- Move to Iteration 2: Add basic 3D rendering
- Add triangle/cube rendering
- Add camera system
- Add basic camera controls (WASD)

---

**Status:** Ready for implementation  
**Last Updated:** 2026-04-19