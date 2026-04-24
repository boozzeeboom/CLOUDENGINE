cmake_minimum_required(VERSION 3.20)

# =============================================================================
# CLOUDENGINE Unit Tests
# =============================================================================

# Enable testing
enable_testing()

# Include directories for tests
# Catch2 is header-only, so we just need the single include path
target_include_directories(CloudEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs/flecs
    ${CMAKE_SOURCE_DIR}/libs/catch2/single_include
    ${CMAKE_SOURCE_DIR}/libs/catch2/src
    ${CMAKE_SOURCE_DIR}/libs/glad/include
    ${CMAKE_SOURCE_DIR}/libs/glfw/include
    ${CMAKE_SOURCE_DIR}/libs/glm-master
    ${CMAKE_SOURCE_DIR}/libs/spdlog/include
    ${CMAKE_SOURCE_DIR}/libs/enet-1.3.18/include
    ${CMAKE_SOURCE_DIR}/libs/jolt
    ${CMAKE_SOURCE_DIR}/libs/jolt/TestFramework
    ${CMAKE_SOURCE_DIR}/libs
)

# Test executable
add_executable(CloudEngineTests
    ecs_components_test.cpp
)

# Link with main engine library (for component definitions)
target_link_libraries(CloudEngineTests
    CloudEngine
)

# Add test to ctest
add_test(NAME CloudEngineTests COMMAND CloudEngineTests)

# Optional: Run tests with verbose output
# ctest --output-on-failure
