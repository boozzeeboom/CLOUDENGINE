#pragma once
#include <glm/glm.hpp>

namespace Core { namespace Rendering {

/// First-person/free-fly camera for navigation.
class Camera {
public:
    Camera();

    void setPosition(const glm::vec3& pos);
    void setRotation(float yaw, float pitch);

    glm::vec3 getPosition() const { return _position; }
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const { return _up; }

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;

    void moveForward(float speed);
    void moveRight(float speed);
    void moveUp(float speed);
    void rotate(float yawDelta, float pitchDelta);

private:
    glm::vec3 _position;
    float _yaw;
    float _pitch;
    glm::vec3 _up;
    glm::vec3 _forward;

    void updateVectors();
};

}} // namespace Core::Rendering
