cmake_minimum_required(VERSION 3.20)
project(CLOUDENGINE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/libs/glfw/include
    ${CMAKE_SOURCE_DIR}/libs/glm-master
    ${CMAKE_SOURCE_DIR}/libs/flecs
    ${CMAKE_SOURCE_DIR}/libs/flecs/src
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
    "libs/flecs/flecs.c"
)

# Executable
add_executable(CloudEngine ${SOURCES} src/main.cpp)

# GLFW build (static)
add_subdirectory(libs/glfw)

# Link libraries
target_link_libraries(CloudEngine
    ${OPENGL_gl_LIBRARY}
    glfw
)
