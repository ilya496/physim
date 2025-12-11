#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/UUID.h"
#include "entt.hpp"

// Core
struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const UUID& id) : ID(id) {}
};

struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const std::string& tag) : Tag(tag) {}
};

struct TransformComponent
{
    glm::vec3 Translation{ 0.0f, 0.0f, 0.0f };
    glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

    TransformComponent() = default;
    TransformComponent(const glm::vec3& t) : Translation(t) {}
    TransformComponent(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
        : Translation(t), Rotation(r), Scale(s) {
    }

    glm::mat4 GetTransform() const
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), Translation);
        transform *= glm::mat4_cast(Rotation);
        transform = glm::scale(transform, Scale);
        return transform;
    }
};

// Render
struct MeshRenderComponent
{

};

struct MaterialComponent
{

};

// struct SpriteRenderComponent
// {

// };

struct CameraComponent
{

};

struct DirectionalLightComponent
{

};

struct PointLightComponent
{

};

// struct SpotLightComponent 
// {

// };

// Physics

struct RigidBodyComponent
{
    enum class BodyType { Static, Dynamic, Kinematic };

    BodyType Type = BodyType::Dynamic;

    // Kinematic: transform is driven externally
    bool FreezeRotation = false;
    bool FreezePosition = false;

    // Linear motion
    glm::vec3 Velocity = glm::vec3(0.0f);
    glm::vec3 Acceleration = glm::vec3(0.0f);
    glm::vec3 Forces = glm::vec3(0.0f);

    // Angular motion
    glm::vec3 AngularVelocity = glm::vec3(0.0f);
    glm::vec3 Torque = glm::vec3(0.0f);

    // Mass & inertia
    float Mass = 1.0f;              // must not be zero for dynamic bodies
    float InverseMass = 1.0f;       // computed internally
    glm::mat3 InertiaTensor = glm::mat3(1.0f);
    glm::mat3 InverseInertiaTensor = glm::mat3(1.0f);

    // Physical material
    float LinearDamping = 0.01f;
    float AngularDamping = 0.01f;

    bool UseGravity = true;

    // Runtime pointer (if needed in future)
    void* RuntimeBody = nullptr;
};

struct BoxColliderComponent
{
    glm::vec3 HalfSize = glm::vec3(0.5f);
    glm::vec3 Offset = glm::vec3(0.0f);

    bool IsTrigger = false;
};

// struct SphereColliderComponent
// {
//     float Radius = 0.5f;
//     glm::vec3 Offset = glm::vec3(0.0f);

//     bool IsTrigger = false;
// };

// struct CapsuleColliderComponent
// {
//     float Radius = 0.5f;
//     float Height = 1.0f;
//     glm::vec3 Offset = glm::vec3(0.0f);

//     bool IsTrigger = false;
// };

// struct MeshColliderComonent
// {

// };

struct PhysicsMaterialComponent
{
    float Friction = 0.5f;
    float Restitution = 0.1f;
    float Density = 1.0f;
};

// struct FixedJointComponent
// {
//     entt::entity ConnectedEntity = entt::null;

//     glm::vec3 Anchor = glm::vec3(0.0f);
//     void* RuntimeJoint = nullptr;
// };

// struct DistanceJointComponent
// {

// };

// struct HingeJointComponent
// {

// };