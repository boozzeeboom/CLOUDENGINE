#include "rendering/camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace Core { namespace Rendering {

Camera::Camera()
    : _position(0.0f, 3000.0f, 0.0f)
    , _yaw(0.0f)
    , _pitch(0.0f)
    , _up(0.0f, 1.0f, 0.0f)
    , _forward(0.0f, 0.0f, -1.0f)
{
    updateVectors();
}

void Camera::setPosition(const glm::vec3& pos) {
    _position = pos;
}

void Camera::setRotation(float yaw, float pitch) {
    _yaw = yaw;
    _pitch = pitch;
    updateVectors();
}

glm::vec3 Camera::getForward() const {
    return _forward;
}

glm::vec3 Camera::getRight() const {
    return glm::normalize(glm::cross(_forward, _up));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(_position, _position + _forward, _up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100000.0f);
}

void Camera::moveForward(float speed) {
    _position += _forward * speed;
}

void Camera::moveRight(float speed) {
    _position += getRight() * speed;
}

void Camera::moveUp(float speed) {
    _position.y += speed;
}

void Camera::rotate(float yawDelta, float pitchDelta) {
    _yaw += yawDelta;
    _pitch += pitchDelta;
    // Ограничение угла тангажа
    _pitch = glm::clamp(_pitch, -1.5f, 1.5f);
    updateVectors();
}

void Camera::updateVectors() {
    // Fixed: sin/cos for X and Z were swapped (matching Engine.cpp formula)
    _forward.x = glm::sin(_yaw) * glm::cos(_pitch);
    _forward.y = glm::sin(_pitch);
    _forward.z = glm::cos(_yaw) * glm::cos(_pitch);
    _forward = glm::normalize(_forward);
}

}} // namespace Core::Rendering
