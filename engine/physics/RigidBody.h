#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

enum class BodyType
{
    Static,
    Kinematic,
    Dynamic
};

struct RigidBody
{
    BodyType Type;

    glm::vec3 Position;
    glm::quat Rotation;

    glm::vec3 LinearVelocity;
    glm::vec3 AngularVelocity;

    glm::vec3 Force;
    glm::vec3 Torque;

    float InverseMass;
    glm::mat3 InverseInertiaLocal;

    bool IsAwake;
};