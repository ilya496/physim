#pragma once

#include <vector>
#include "scene/Scene.h"

struct PhysicsBody
{
    entt::entity Entity;

    glm::vec3 Position;
    glm::quat Rotation;

    glm::vec3 Velocity;
    glm::vec3 AngularVelocity;

    glm::vec3 HalfExtents; // world-scaled

    float InverseMass;

    glm::mat3 InertiaLocal;
    glm::mat3 InverseInertiaLocal;
    glm::mat3 InverseInertiaWorld;

    float Restitution;
    float Friction;

    bool IsStatic;
};

struct ContactPoint
{
    glm::vec3 Position;
    float Penetration;
};

struct ContactManifold
{
    PhysicsBody* A;
    PhysicsBody* B;

    glm::vec3 Normal;
    ContactPoint Contacts[4];
    uint32_t ContactCount = 0;
    float AccumulatedImpulse = 0.0f;
};

struct AABB
{
    glm::vec3 Min;
    glm::vec3 Max;
};

struct BroadphasePair
{
    PhysicsBody* A;
    PhysicsBody* B;
};

class PhysicsWorld
{
public:
    PhysicsWorld(Scene* scene);
    ~PhysicsWorld();

    void Step(float dt);
    void Shutdown();

private:
    void BuildBodies();
    void IntegrateForces(float dt);
    void Broadphase();
    void Narrowphase();
    void SolveContacts();
    void IntegrateVelocities(float dt);
    void WriteBack();

private:
    Scene* m_Scene = nullptr;

    std::vector<PhysicsBody> m_Bodies;
    std::vector<BroadphasePair> m_Pairs;
    std::vector<ContactManifold> m_Contacts;

    const glm::vec3 m_Gravity = { 0.0f, -9.81f, 0.0f };
};