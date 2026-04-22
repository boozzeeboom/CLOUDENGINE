cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Jolt Physics — Local Library
# Repository: https://github.com/jrouwe/JoltPhysics
# Note: Clone to libs/jolt manually: git clone https://github.com/jrouwe/JoltPhysics.git libs/jolt
# Build folder: libs/jolt/Build/ contains CMakeLists.txt
# =============================================================================

# Force Jolt to use dynamic MSVC runtime library (/MD) to match CLOUDENGINE
# Jolt option: USE_STATIC_MSVC_RUNTIME_LIBRARY (default ON = static /MT)
# Set to OFF to use dynamic /MD
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "Use static MSVC runtime library" FORCE)

# Jolt build (local) — use Build subfolder
add_subdirectory(libs/jolt/Build)

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

# Source files — C++ sources via GLOB_RECURSE
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
)

# Copy shaders to build directory (works for both build/ and build/Debug/)
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_BINARY_DIR}/shaders/")
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_BINARY_DIR}/Debug/shaders/")

# GLFW build (static library)
add_subdirectory(libs/glfw)

# Link libraries
target_link_libraries(CloudEngine PRIVATE
    glfw
    ${OPENGL_gl_LIBRARY}
    winmm.lib   # ENet time functions
    Jolt::Jolt  # Jolt Physics
)
