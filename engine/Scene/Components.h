#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/UUID.h"
#include "asset/Asset.h"
#include "render/Camera.h"
#include "render/LightType.h"
#include "physics/AABB.h"

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
    // glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::quat Rotation = glm::identity<glm::quat>();
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

    TransformComponent() = default;
    TransformComponent(const glm::vec3& t) : Translation(t) {}
    TransformComponent(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
        : Translation(t), Rotation(r), Scale(s) {
    }

    glm::mat4 GetTransform() const
    {
        return glm::translate(glm::mat4(1.0f), Translation)
            * glm::mat4_cast(Rotation)
            * glm::scale(glm::mat4(1.0f), Scale);
    }
};

struct AABBComponent
{
    AABB WorldAABB;
    bool Dirty = true; // idk if needed
};

// Render
struct MeshRenderComponent
{
    AssetHandle Mesh;
    AssetHandle Material;
};

struct LightComponent
{
    LightType Type = LightType::Point;
    glm::vec3 Color{ 1.0f };
    float Intensity = 1.0f;

    float Range = 10.0f; // point / spot light
    glm::vec3 Direction{ 0.0f, -1.0f, 0.0f }; // directional light
};

struct RigidBodyComponent
{
    float Mass = 1.0f;
    float Restitution = 0.2f;
    float Friction = 0.6f;
    bool IsStatic = false;

    void* RuntimeBody = nullptr;
    // glm::vec3 Velocity{ 0 };
    // glm::vec3 AngularVelocity{ 0 };
};


enum class ColliderType
{
    Sphere,
    Box,
    Convex
};

struct SphereColliderComponent
{
    float Radius = 0.5f;
};

struct BoxColliderComponent
{
    glm::vec3 HalfExtents{ 0.5f };
};

struct DistanceJointComponent
{
    entt::entity ConnectedEntity;

    glm::vec3 LocalAnchorA{ 0.0f };
    glm::vec3 LocalAnchorB{ 0.0f };

    float TargetLength = 0.0f;
};

// struct HingeJointComponent
// {
//     entt::entity ConnectedEntity;

//     glm::vec3 LocalAnchorA{ 0.0f };
//     glm::vec3 LocalAnchorB{ 0.0f };

//     glm::vec3 LocalAxisA{ 0,1,0 };
// };