#pragma once

#include <vector>
#include "scene/Scene.h"

enum class BodyType
{
    Static, Kinematic, Dynamic
};

struct AABB
{
    glm::vec3 Min;
    glm::vec3 Max;

    bool Overlaps(const AABB& other) const
    {
        return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
            (Min.y <= other.Max.y && Max.y >= other.Min.y) &&
            (Min.z <= other.Max.z && Max.z >= other.Min.z);
    }
};

struct RigidBody
{
    BodyType Type;

    glm::vec3 Position;
    glm::quat Rotation;

    glm::vec3 LinearVelocity;
    glm::vec3 AngularVelocity;

    float InverseMass;
    glm::mat3 InverseInertiaLocal;

    glm::vec3 Force;
    glm::vec3 Torque;

    float Restitution;
    float Friction;
};

enum class ShapeType
{
    Sphere,
    Box,
    Capsule,
    ConvexHull,
    TriangleMesh
};

struct Shape
{
    ShapeType Type;
    AABB LocalAABB;

    virtual ~Shape() = default;
};

struct SphereShape : public Shape
{
    float Radius = 0.5f;

    explicit SphereShape(float radius)
        : Radius(radius)
    {
        Type = ShapeType::Sphere;
        LocalAABB.Min = glm::vec3(-Radius);
        LocalAABB.Max = glm::vec3(Radius);
    }
};

struct BoxShape : public Shape
{
    glm::vec3 HalfExtents{ 0.5f };

    explicit BoxShape(const glm::vec3& halfExtents)
        : HalfExtents(halfExtents)
    {
        Type = ShapeType::Box;
        LocalAABB.Min = -HalfExtents;
        LocalAABB.Max = HalfExtents;
    }
};

struct CapsuleShape : public Shape
{
    float Radius = 0.5f;
    float HalfHeight = 1.0f;

    CapsuleShape(float radius, float height)
        : Radius(radius), HalfHeight(height * 0.5f)
    {
        Type = ShapeType::Capsule;

        LocalAABB.Min = glm::vec3(
            -Radius,
            -HalfHeight - Radius,
            -Radius
        );

        LocalAABB.Max = glm::vec3(
            Radius,
            HalfHeight + Radius,
            Radius
        );
    }
};

struct ConvexHullShape : public Shape
{
    std::vector<glm::vec3> Vertices;

    explicit ConvexHullShape(const std::vector<glm::vec3>& vertices)
        : Vertices(vertices)
    {
        Type = ShapeType::ConvexHull;

        glm::vec3 min = vertices[0];
        glm::vec3 max = vertices[0];

        for (const auto& v : vertices)
        {
            min = glm::min(min, v);
            max = glm::max(max, v);
        }

        LocalAABB.Min = min;
        LocalAABB.Max = max;
    }
};

struct TriangleMeshShape : public Shape
{
    std::vector<glm::vec3> Vertices;
    std::vector<uint32_t> Indices;

    TriangleMeshShape(
        const std::vector<glm::vec3>& vertices,
        const std::vector<uint32_t>& indices)
        : Vertices(vertices), Indices(indices)
    {
        Type = ShapeType::TriangleMesh;

        glm::vec3 min = vertices[0];
        glm::vec3 max = vertices[0];

        for (const auto& v : vertices)
        {
            min = glm::min(min, v);
            max = glm::max(max, v);
        }

        LocalAABB.Min = min;
        LocalAABB.Max = max;
    }
};


struct Collider
{
    Shape* ColissionShape = nullptr;
    AABB WorldAABB;

    int32_t BroadPhaseProxy = -1;

    uint32_t ColissionLayer = 0x00000001;
    uint32_t ColissionMask = 0xFFFFFFFF;
};

struct BroadPhasePair
{
    int32_t ProxyA;
    int32_t ProxyB;
};

class BroadPhase
{
public:
    virtual ~BroadPhase() = default;

    virtual int32_t CreateProxy(const AABB& aabb, void* userData) = 0;
    virtual void DestroyProxy(int32_t proxyId) = 0;
    virtual void MoveProxy(int32_t proxyId, const AABB& aabb) = 0;

    virtual void ComputePairs(std::vector<BroadPhasePair>& outPairs) = 0;
};

struct TreeNode
{
    AABB Box;

    int32_t Parent = -1;
    int32_t Left = -1;
    int32_t Right = -1;

    int32_t Height = 0;

    void* UserData = nullptr;

    bool IsLeaf() const
    {
        return Left == -1;
    }
};

class DynamicAABBTree : public BroadPhase
{
public:
    int32_t CreateProxy(const AABB& aabb, void* userData) override;
    void DestroyProxy(int32_t proxyId) override;
    void MoveProxy(int32_t proxyId, const AABB& aabb) override;
    void ComputePairs(std::vector<BroadPhasePair>& outPairs) override;

private:
    int32_t AllocateNode();
    void FreeNode(int32_t node);

    void InsertLeaf(int32_t leaf);
    void DeleteLeaf(int32_t leaf);

    int32_t Balance(int32_t node);

private:
    std::vector<TreeNode> m_Nodes;
    int32_t m_Root = -1;
    int32_t m_FreeList = -1;
};

struct ContactPoint
{
    glm::vec3 Position{ 0.0f };
    float Penetration = 0.0f;
};

struct ContactManifold
{
    RigidBody* BodyA;
    RigidBody* BodyB;

    glm::vec3 Normal{ 0.0f };

    ContactPoint Contacts[4];
    uint32_t ContactCount = 0;
};

class NarrowPhase
{
public:
    bool GenerateContacts(
        const Shape* shapeA,
        const Shape* shapeB,
        ContactManifold& manifold);
};

class Integrator
{
public:
    void Integrate(RigidBody& body, float dt)
    {
        if (body.Type != BodyType::Dynamic)
            return;

        body.LinearVelocity += body.Force * body.InverseMass * dt;
        body.Position += body.LinearVelocity * dt;

        body.Force = glm::vec3(0.0f);
        body.Torque = glm::vec3(0.0f);
    }
};

struct SoftBodyNode
{
    glm::vec3 Position{ 0.0f };
    glm::vec3 Velocity{ 0.0f };
    float InverseMass = 0.0f;
};

struct SoftBodyLink
{
    uint32_t NodeA;
    uint32_t NodeB;
    float RestLength = 0.0f;
    float Stiffness = 1.0f;
};

struct SoftBody
{
    std::vector<SoftBodyNode> Nodes;
    std::vector<SoftBodyLink> Links;
};

struct ContactContraint
{
    RigidBody* BodyA;
    RigidBody* BodyB;

    glm::vec3 Normal;
    glm::vec3 ContactPoint;

    float Penetration;
    float NormalMass;
    float Bias;

    float AccumulatedImpulse = 0.0f;
};

class ContactSolver
{
public:
    void Solve(
        std::vector<ContactManifold>& manifolds,
        float dt,
        int iterations = 10
    );
};

class PhysicsWorld
{
public:
    explicit PhysicsWorld(Scene* scene);
    ~PhysicsWorld();

    void Step(float dt);
    void Shutdown();

    RigidBody& CreateRigidBody();
    Collider& CreateCollider(RigidBody& body, Shape* shape);

private:
    Scene* m_Scene = nullptr;

    std::vector<RigidBody> m_RigidBodies;
    std::vector<Collider> m_Colliders;

    std::vector<ContactManifold> m_ContactManifolds;

    BroadPhase* m_BroadPhase = nullptr;
    NarrowPhase m_NarrowPhase;
    Integrator m_Integrator;

    const glm::vec3 m_Gravity = { 0.0f, -9.81f, 0.0f };
};