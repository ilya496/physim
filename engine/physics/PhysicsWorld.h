#pragma once

#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AABB.h"

enum class BodyType
{
    Static, Dynamic
};

enum class ShapeType
{
    Sphere,
    Box,
    TriangleMesh
};

struct Shape
{
    ShapeType Type;
    virtual ~Shape() = default;
    virtual AABB CalculateLocalAABB() const = 0;
    virtual glm::mat3 CalculateInertiaTensor(float mass) const = 0;
};

struct SphereShape : public Shape
{
    float Radius;
    SphereShape(float r) : Radius(r) { Type = ShapeType::Sphere; }

    AABB CalculateLocalAABB() const override {
        return { glm::vec3(-Radius), glm::vec3(Radius) };
    }

    glm::mat3 CalculateInertiaTensor(float mass) const override {
        float I = (2.0f / 5.0f) * mass * Radius * Radius;
        return glm::mat3(I);
    }
};

struct BoxShape : public Shape
{
    glm::vec3 HalfExtents;
    BoxShape(const glm::vec3& half) : HalfExtents(half) { Type = ShapeType::Box; }

    AABB CalculateLocalAABB() const override {
        return { -HalfExtents, HalfExtents };
    }

    glm::mat3 CalculateInertiaTensor(float mass) const override {
        float w2 = (2.0f * HalfExtents.x) * (2.0f * HalfExtents.x);
        float h2 = (2.0f * HalfExtents.y) * (2.0f * HalfExtents.y);
        float d2 = (2.0f * HalfExtents.z) * (2.0f * HalfExtents.z);

        return glm::mat3(
            (1.0f / 12.0f) * mass * (h2 + d2), 0, 0,
            0, (1.0f / 12.0f) * mass * (w2 + d2), 0,
            0, 0, (1.0f / 12.0f) * mass * (w2 + h2)
        );
    }
};

struct TriangleMeshShape : public Shape
{
    std::vector<glm::vec3> Vertices;
    std::vector<uint32_t> Indices;
    AABB CachedAABB;

    TriangleMeshShape(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& inds)
        : Vertices(verts), Indices(inds)
    {
        Type = ShapeType::TriangleMesh;

        CachedAABB.Min = Vertices[0];
        CachedAABB.Max = Vertices[0];
        for (const auto& v : Vertices) {
            CachedAABB.Min = glm::min(CachedAABB.Min, v);
            CachedAABB.Max = glm::max(CachedAABB.Max, v);
        }
    }

    AABB CalculateLocalAABB() const override { return CachedAABB; }

    // Meshes are usually static, so infinite inertia (zero inverse)
    glm::mat3 CalculateInertiaTensor(float mass) const override { return glm::mat3(0.0f); }
};

struct RigidBody
{
    int ID = -1;
    BodyType Type = BodyType::Dynamic;

    // State
    glm::vec3 Position{ 0.0f };
    glm::quat Orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

    glm::vec3 LinearVelocity{ 0.0f };
    glm::vec3 AngularVelocity{ 0.0f };

    // Properties
    float Mass = 1.0f;
    float InverseMass = 1.0f;

    glm::mat3 InverseInertiaLocal{ 1.0f };
    glm::mat3 InverseInertiaWorld{ 1.0f };

    float Friction = 0.5f;
    float Restitution = 0.2f; // Bounciness

    // Forces
    glm::vec3 ForceAccumulator{ 0.0f };
    glm::vec3 TorqueAccumulator{ 0.0f };

    Shape* CollisionShape = nullptr;
    AABB WorldAABB;

    void SetStatic()
    {
        Type = BodyType::Static;
        InverseMass = 0.0f;
        InverseInertiaLocal = glm::mat3(0.0f);
        InverseInertiaWorld = glm::mat3(0.0f);
        LinearVelocity = glm::vec3(0.0f);
        AngularVelocity = glm::vec3(0.0f);
    }

    void RecalculateMassProperties()
    {
        if (Type == BodyType::Static || !CollisionShape) {
            SetStatic();
            return;
        }
        InverseMass = 1.0f / Mass;
        glm::mat3 I = CollisionShape->CalculateInertiaTensor(Mass);
        InverseInertiaLocal = glm::inverse(I);
    }

    void UpdateWorldInertia()
    {
        if (Type == BodyType::Static) return;
        glm::mat3 R = glm::toMat3(Orientation);
        InverseInertiaWorld = R * InverseInertiaLocal * glm::transpose(R);
    }

    void UpdateAABB()
    {
        if (!CollisionShape) return;

        AABB local = CollisionShape->CalculateLocalAABB();
        glm::vec3 center = Position;
        glm::vec3 extent = (local.Max - local.Min) * 0.5f;

        // 1. Get the rotation matrix
        glm::mat3 rotationMat = glm::toMat3(Orientation);

        // 2. Get absolute values of the matrix (Column-by-column)
        glm::mat3 R;
        R[0] = glm::abs(rotationMat[0]);
        R[1] = glm::abs(rotationMat[1]);
        R[2] = glm::abs(rotationMat[2]);

        // 3. Compute new extent
        glm::vec3 newExtent = R * extent;

        WorldAABB.Min = center - newExtent;
        WorldAABB.Max = center + newExtent;
    }

    glm::vec3 LocalToWorld(const glm::vec3& local) const
    {
        return Position + (Orientation * local);
    }

    glm::vec3 WorldToLocal(const glm::vec3& world) const
    {
        return glm::inverse(Orientation) * (world - Position);
    }
};

struct Contact
{
    glm::vec3 WorldPointA;
    glm::vec3 WorldPointB;
    float Depth;
};

struct Manifold
{
    RigidBody* BodyA;
    RigidBody* BodyB;
    glm::vec3 Normal; // From A to B
    std::vector<Contact> Contacts;
};

inline bool IntersectSphereSphere(RigidBody* a, RigidBody* b, Manifold& m)
{
    SphereShape* sA = (SphereShape*)a->CollisionShape;
    SphereShape* sB = (SphereShape*)b->CollisionShape;

    glm::vec3 delta = b->Position - a->Position;
    float distSq = glm::length2(delta);
    float radiusSum = sA->Radius + sB->Radius;

    if (distSq > radiusSum * radiusSum) return false;

    float dist = std::sqrt(distSq);

    m.Normal = (dist > 0.0001f) ? delta / dist : glm::vec3(0, 1, 0);
    m.BodyA = a;
    m.BodyB = b;

    Contact c;
    c.Depth = radiusSum - dist;
    c.WorldPointA = a->Position + m.Normal * sA->Radius;
    c.WorldPointB = b->Position - m.Normal * sB->Radius;
    m.Contacts.push_back(c);

    return true;
}

inline bool IntersectSphereBox(RigidBody* sphereBody, RigidBody* boxBody, Manifold& m)
{
    SphereShape* s = (SphereShape*)sphereBody->CollisionShape;
    BoxShape* b = (BoxShape*)boxBody->CollisionShape;

    // Transform sphere center to box local space
    glm::vec3 localSpherePos = boxBody->WorldToLocal(sphereBody->Position);

    // Clamp point to box extents
    glm::vec3 closestPoint = glm::clamp(localSpherePos, -b->HalfExtents, b->HalfExtents);

    float distSq = glm::length2(localSpherePos - closestPoint);
    if (distSq > s->Radius * s->Radius) return false;

    // We have a collision
    m.BodyA = sphereBody;
    m.BodyB = boxBody;

    glm::vec3 localNormal = localSpherePos - closestPoint;
    float dist = std::sqrt(distSq);

    // Handle sphere inside box
    if (dist < 0.0001f)
    {
        // Find closest axis to push out
        // (Simplified for brevity: assume pushing up Y for now or use SAT logic)
        localNormal = glm::vec3(0, 1, 0);
        dist = 0.0f; // Deep penetration
    }
    else
    {
        localNormal /= dist;
    }

    m.Normal = boxBody->Orientation * localNormal; // World normal

    Contact c;
    c.Depth = s->Radius - dist;
    // World position of contact on Box
    c.WorldPointB = boxBody->LocalToWorld(closestPoint);
    // World position of contact on Sphere
    c.WorldPointA = sphereBody->Position - m.Normal * s->Radius;
    m.Contacts.push_back(c);

    return true;
}

// --- SAT Helpers for Box-Box ---
inline float GetOverlap(float minA, float maxA, float minB, float maxB)
{
    return std::min(maxA, maxB) - std::max(minA, minB);
}

inline float GetProjectedOverlap(const glm::vec3& axis, BoxShape* boxA, const glm::quat& rotA, const glm::vec3& posA,
    BoxShape* boxB, const glm::quat& rotB, const glm::vec3& posB)
{
    // Project Box A
    glm::vec3 axesA[3] = { rotA * glm::vec3(1,0,0), rotA * glm::vec3(0,1,0), rotA * glm::vec3(0,0,1) };
    float rA = boxA->HalfExtents.x * std::abs(glm::dot(axesA[0], axis)) +
        boxA->HalfExtents.y * std::abs(glm::dot(axesA[1], axis)) +
        boxA->HalfExtents.z * std::abs(glm::dot(axesA[2], axis));

    // Project Box B
    glm::vec3 axesB[3] = { rotB * glm::vec3(1,0,0), rotB * glm::vec3(0,1,0), rotB * glm::vec3(0,0,1) };
    float rB = boxB->HalfExtents.x * std::abs(glm::dot(axesB[0], axis)) +
        boxB->HalfExtents.y * std::abs(glm::dot(axesB[1], axis)) +
        boxB->HalfExtents.z * std::abs(glm::dot(axesB[2], axis));

    float projectionDist = std::abs(glm::dot(posB - posA, axis));
    return (rA + rB) - projectionDist;
}

inline bool IntersectBoxBox(RigidBody* a, RigidBody* b, Manifold& m)
{
    BoxShape* boxA = (BoxShape*)a->CollisionShape;
    BoxShape* boxB = (BoxShape*)b->CollisionShape;

    glm::vec3 axesA[3] = { a->Orientation * glm::vec3(1,0,0), a->Orientation * glm::vec3(0,1,0), a->Orientation * glm::vec3(0,0,1) };
    glm::vec3 axesB[3] = { b->Orientation * glm::vec3(1,0,0), b->Orientation * glm::vec3(0,1,0), b->Orientation * glm::vec3(0,0,1) };

    glm::vec3 testAxes[15];
    int axisCount = 0;

    // Face axes
    for (int i = 0; i < 3; i++) testAxes[axisCount++] = axesA[i];
    for (int i = 0; i < 3; i++) testAxes[axisCount++] = axesB[i];

    // Edge-Edge cross products
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            glm::vec3 cross = glm::cross(axesA[i], axesB[j]);
            if (glm::length2(cross) > 0.001f)
            {
                testAxes[axisCount++] = glm::normalize(cross);
            }
        }
    }

    float minOverlap = FLT_MAX;
    glm::vec3 bestAxis;

    for (int i = 0; i < axisCount; i++)
    {
        float overlap = GetProjectedOverlap(testAxes[i], boxA, a->Orientation, a->Position, boxB, b->Orientation, b->Position);
        if (overlap < 0) return false; // Separating axis found

        if (overlap < minOverlap)
        {
            minOverlap = overlap;
            bestAxis = testAxes[i];
        }
    }

    // Ensure normal points from A to B
    if (glm::dot(bestAxis, b->Position - a->Position) < 0)
    {
        bestAxis = -bestAxis;
    }

    m.BodyA = a;
    m.BodyB = b;
    m.Normal = bestAxis;

    Contact c;
    c.Depth = minOverlap;
    // Approximating contact point (Simpler than full clipping for this snippet)
    c.WorldPointA = a->Position + (bestAxis * boxA->HalfExtents.x); // Rough approximation
    c.WorldPointB = b->Position - (bestAxis * boxB->HalfExtents.x);
    m.Contacts.push_back(c);

    return true;
}

inline void Dispatch(RigidBody* a, RigidBody* b, std::vector<Manifold>& manifolds)
{
    if (a->Type == BodyType::Static && b->Type == BodyType::Static) return;

    Manifold m;
    bool collided = false;

    // Very basic double dispatch
    if (a->CollisionShape->Type == ShapeType::Sphere && b->CollisionShape->Type == ShapeType::Sphere)
    {
        collided = IntersectSphereSphere(a, b, m);
    }
    else if (a->CollisionShape->Type == ShapeType::Sphere && b->CollisionShape->Type == ShapeType::Box)
    {
        collided = IntersectSphereBox(a, b, m);
    }
    else if (a->CollisionShape->Type == ShapeType::Box && b->CollisionShape->Type == ShapeType::Sphere)
    {
        collided = IntersectSphereBox(b, a, m); // Flip
        if (collided) m.Normal = -m.Normal;
    }
    else if (a->CollisionShape->Type == ShapeType::Box && b->CollisionShape->Type == ShapeType::Box)
    {
        collided = IntersectBoxBox(a, b, m);
    }
    // Mesh collisions would loop over triangles here and perform Triangle-Sphere or Triangle-Box tests

    if (collided)
    {
        manifolds.push_back(m);
    }
}

struct BodyState {
    glm::vec3 Position;
    glm::quat Orientation;
    glm::vec3 LinearVelocity;
    glm::vec3 AngularVelocity;
};

// Map of EntityID -> State
using PhysicsSnapshot = std::map<uint32_t, BodyState>;

class PhysicsWorld
{
public:
    glm::vec3 Gravity{ 0.0f, -9.81f, 0.0f };
    std::vector<RigidBody*> Bodies;
    std::vector<Manifold> Contacts;

    ~PhysicsWorld()
    {
        for (auto b : Bodies) delete b;
    }

    RigidBody* CreateBody(const glm::vec3& pos, Shape* shape, float mass = 1.0f)
    {
        RigidBody* body = new RigidBody();
        body->Position = pos;
        body->CollisionShape = shape;
        body->Mass = mass;
        body->RecalculateMassProperties();
        body->UpdateWorldInertia();
        body->UpdateAABB();
        Bodies.push_back(body);
        return body;
    }

    void SetState(const PhysicsSnapshot& snapshot)
    {
        for (auto* body : Bodies)
        {
            if (snapshot.find(body->ID) != snapshot.end())
            {
                const auto& state = snapshot.at(body->ID);
                body->Position = state.Position;
                body->Orientation = state.Orientation;
                body->LinearVelocity = state.LinearVelocity;
                body->AngularVelocity = state.AngularVelocity;
                body->UpdateAABB();
                body->UpdateWorldInertia();
            }
        }
    }

    void Step(float dt)
    {
        // 1. Integrate Forces (Gravity)
        for (auto b : Bodies) {
            if (b->Type == BodyType::Dynamic) {
                b->LinearVelocity += (b->ForceAccumulator * b->InverseMass + Gravity) * dt;
                b->AngularVelocity += (b->InverseInertiaWorld * b->TorqueAccumulator) * dt;

                // Damping
                b->LinearVelocity *= 0.98f;
                b->AngularVelocity *= 0.98f;
            }
            b->ForceAccumulator = glm::vec3(0.0f);
            b->TorqueAccumulator = glm::vec3(0.0f);
        }

        // 2. Broadphase & Narrowphase
        Contacts.clear();
        // Naive O(N^2) loop for simplicity. 
        // In production, replace this with your DynamicAABBTree queries.
        for (size_t i = 0; i < Bodies.size(); ++i)
        {
            for (size_t j = i + 1; j < Bodies.size(); ++j)
            {
                RigidBody* A = Bodies[i];
                RigidBody* B = Bodies[j];

                if (A->Type == BodyType::Static && B->Type == BodyType::Static) continue;

                if (A->WorldAABB.Overlaps(B->WorldAABB)) {
                    Dispatch(A, B, Contacts);
                }
            }
        }

        // 3. Solve Constraints (Sequential Impulses)
        for (int i = 0; i < 10; ++i)
        { // 10 Iterations
            SolveConstraints();
        }

        // 4. Integrate Velocities
        for (auto b : Bodies)
        {
            if (b->Type == BodyType::Dynamic)
            {
                b->Position += b->LinearVelocity * dt;

                glm::quat q = glm::quat(0.0f, b->AngularVelocity.x, b->AngularVelocity.y, b->AngularVelocity.z);
                b->Orientation += (q * b->Orientation) * 0.5f * dt;
                b->Orientation = glm::normalize(b->Orientation);

                b->UpdateWorldInertia();
                b->UpdateAABB();
            }
        }
    }

private:
    void SolveConstraints()
    {
        for (auto& m : Contacts)
        {
            RigidBody* A = m.BodyA;
            RigidBody* B = m.BodyB;

            for (auto& c : m.Contacts) {
                glm::vec3 rA = c.WorldPointA - A->Position;
                glm::vec3 rB = c.WorldPointB - B->Position;

                // Relative Velocity
                glm::vec3 vA = A->LinearVelocity + glm::cross(A->AngularVelocity, rA);
                glm::vec3 vB = B->LinearVelocity + glm::cross(B->AngularVelocity, rB);
                glm::vec3 vRel = vB - vA;

                // 1. Penetration Correction (Positional) - "Baumgarte Stabilization"
                // Stops objects from sinking, pushes them apart slightly based on depth
                float beta = 0.2f; // Correction percentage
                float slop = 0.01f; // Penetration allowance
                float bias = (beta / 0.016f) * std::max(0.0f, c.Depth - slop);

                // 2. Velocity Solver
                float velAlongNormal = glm::dot(vRel, m.Normal);

                // Do not resolve if velocities are separating
                if (velAlongNormal > 0) continue;

                float restitution = std::min(A->Restitution, B->Restitution);
                float jv = -(1.0f + restitution) * velAlongNormal + bias; // +bias handles the sinking

                // Effective Mass (Inverse)
                glm::vec3 raxn = glm::cross(rA, m.Normal);
                glm::vec3 rbxn = glm::cross(rB, m.Normal);

                float invMassSum = A->InverseMass + B->InverseMass +
                    glm::dot(raxn, A->InverseInertiaWorld * raxn) +
                    glm::dot(rbxn, B->InverseInertiaWorld * rbxn);

                float lambda = jv / invMassSum;

                glm::vec3 impulse = m.Normal * lambda;

                // Apply Impulse
                if (A->Type == BodyType::Dynamic) {
                    A->LinearVelocity -= impulse * A->InverseMass;
                    A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(rA, impulse);
                }
                if (B->Type == BodyType::Dynamic) {
                    B->LinearVelocity += impulse * B->InverseMass;
                    B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rB, impulse);
                }

                // Note: Friction impulses would go here (Tangent calculations)
            }
        }
    }
};