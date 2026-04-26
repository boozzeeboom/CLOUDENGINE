cmake_minimum_required(VERSION 3.20)

# =============================================================================
# CLOUDENGINE — Custom Game Engine for Project C: The Clouds
# =============================================================================

# Find OpenGL (required for glad)
find_package(OpenGL REQUIRED)

# =============================================================================
# Jolt Physics Configuration вЂ” MUST be set BEFORE add_subdirectory
# Repository: https://github.com/jrouwe/JoltPhysics
# =============================================================================

# CRITICAL: Disable INTERPROCEDURAL_OPTIMIZATION (LTCG) for Jolt
# - Jolt default is ON, which causes ABI mismatch when CLOUDENGINE uses /GL:OFF
# - Setting variables BEFORE add_subdirectory ensures Jolt uses our settings
# See: libs/jolt/Build/CMakeLists.txt lines 32-35

# Disable interprocedural optimizations (link-time code generation)
# This MUST be before add_subdirectory(jolt/Build) to take effect
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "Enable interprocedural optimizations" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# Force Jolt to use dynamic MSVC runtime library (/MD) to match CLOUDENGINE
# Jolt default: ON (static /MT), we want OFF (dynamic /MD)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "Use static MSVC runtime library" FORCE)

# Disable exceptions and RTTI for better performance (Jolt doesn't use them)
set(CPP_EXCEPTIONS_ENABLED OFF CACHE BOOL "Enable C++ exceptions" FORCE)
set(CPP_RTTI_ENABLED OFF CACHE BOOL "Enable C++ RTTI" FORCE)

# Enable assertions in Debug builds for better diagnostics
# In Release, assertions are controlled by JPH_DEBUG define
set(USE_ASSERTS ON CACHE BOOL "Enable asserts" FORCE)

# tinygltf v3 — Header-only glTF 2.0 loader (headers only, no build)
# Include path added below

# =============================================================================
# Project Definition
# =============================================================================
project(CLOUDENGINE VERSION 0.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug/Release compile definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
    message(STATUS "Build type: Debug")
    add_compile_definitions(CE_DEBUG SPDLOG_ACTIVE_LEVEL=0)
else()
    message(STATUS "Build type: Release")
    add_compile_definitions(NDEBUG SPDLOG_ACTIVE_LEVEL=3)
endif()

# Source files вЂ” C++ sources via GLOB_RECURSE
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(APPEND SOURCES 
    "libs/glad/src/glad.c"
    "libs/flecs/flecs.c"
    "libs/enet-1.3.18/callbacks.c"
    "libs/enet-1.3.18/compress.c"
    "libs/enet-1.3.18/host.c"
    "libs/enet-1.3.18/list.c"
    "libs/enet-1.3.18/packet.c"
    "libs/enet-1.3.18/peer.c"
    "libs/enet-1.3.18/protocol.c"
    "libs/enet-1.3.18/unix.c"
    "libs/enet-1.3.18/win32.c"
)

# Executable
add_executable(CloudEngine ${SOURCES})

# GLAD: Only glad.c needs GLAD_GLAPI_EXPORT_BUILD for static linking
# This makes glad.c export the function pointers, others just reference them
set_source_files_properties("libs/glad/src/glad.c" PROPERTIES
    COMPILE_DEFINITIONS "GLAD_GLAPI_EXPORT_BUILD"
)

# Include directories
target_include_directories(CloudEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs/glad/include
    ${CMAKE_SOURCE_DIR}/libs/flecs
    ${CMAKE_SOURCE_DIR}/libs/glfw/include
    ${CMAKE_SOURCE_DIR}/libs/glm-master
    ${CMAKE_SOURCE_DIR}/libs/spdlog/include
    ${CMAKE_SOURCE_DIR}/libs/enet-1.3.18/include    # ENet headers
    ${CMAKE_SOURCE_DIR}/libs/jolt/                   # Jolt headers root (for #include <Jolt/...>)
    ${CMAKE_SOURCE_DIR}/libs/jolt/TestFramework/     # Jolt test framework
    ${CMAKE_SOURCE_DIR}/libs                        # stb_truetype.h
    ${CMAKE_SOURCE_DIR}/libs/tinygltf               # tinygltf v3
)

# Copy shaders and fonts to build directory (works for both build/ and build/Debug/)
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_BINARY_DIR}/shaders/")
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_BINARY_DIR}/Debug/shaders/")
file(COPY "${CMAKE_SOURCE_DIR}/data/fonts/" DESTINATION "${CMAKE_BINARY_DIR}/data/fonts/")
file(COPY "${CMAKE_SOURCE_DIR}/data/fonts/" DESTINATION "${CMAKE_BINARY_DIR}/Debug/data/fonts/")

# Unit tests (Catch2) - DISABLED for UI iteration
# add_subdirectory(tests)

# GLFW build (static library)
add_subdirectory(libs/glfw)

# Jolt Physics build (static library) - MUST be before target_link_libraries
add_subdirectory(libs/jolt/Build)

# Link libraries
target_link_libraries(CloudEngine PRIVATE
    glfw
    ${OPENGL_gl_LIBRARY}
    winmm.lib   # ENet time functions
    Jolt::Jolt  # Jolt Physics
)
