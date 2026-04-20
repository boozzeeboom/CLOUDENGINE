cmake_minimum_required(VERSION 3.20)
project(CLOUDENGINE VERSION 0.2.0 LANGUAGES C CXX)

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
)

# Executable
add_executable(CloudEngine ${SOURCES})

# Include directories
target_include_directories(CloudEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs/glad/include
    ${CMAKE_SOURCE_DIR}/libs/flecs
    ${CMAKE_SOURCE_DIR}/libs/glfw/include
    ${CMAKE_SOURCE_DIR}/libs/glm-master
    ${CMAKE_SOURCE_DIR}/libs/spdlog/include
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
)
