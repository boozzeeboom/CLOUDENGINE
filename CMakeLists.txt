cmake_minimum_required(VERSION 3.20)
project(CLOUDENGINE VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Copy shaders to build directory
file(COPY "${CMAKE_SOURCE_DIR}/shaders/" DESTINATION "${CMAKE_BINARY_DIR}/shaders/")

# GLFW build (static library)
add_subdirectory(libs/glfw)

# Link libraries
target_link_libraries(CloudEngine PRIVATE
    glfw
    ${OPENGL_gl_LIBRARY}
)
