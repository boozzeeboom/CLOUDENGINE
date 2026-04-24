#pragma once
#include <flecs.h>
#include <glm/glm.hpp>

namespace CloudEngineTest {

inline flecs::world create_test_world() {
    return flecs::world();
}

inline bool approximately_equal(float a, float b, float epsilon = 0.001f) {
    return glm::abs(a - b) < epsilon;
}

inline bool vectors_approximately_equal(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.001f) {
    return glm::length(a - b) < epsilon;
}

} // namespace CloudEngineTest
