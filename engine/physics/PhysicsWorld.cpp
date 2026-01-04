#include "PhysicsWorld.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>

static AABB ComputeAABBFromOBB(const PhysicsBody& body)
{
    // Build 8 corner points of the box in world space
    glm::mat3 rotMat = glm::mat3_cast(body.Rotation);
    glm::vec3 hx = body.HalfExtents;

    glm::vec3 corners[8] = {
        {+hx.x, +hx.y, +hx.z}, {+hx.x, +hx.y, -hx.z},
        {+hx.x, -hx.y, +hx.z}, {+hx.x, -hx.y, -hx.z},
        {-hx.x, +hx.y, +hx.z}, {-hx.x, +hx.y, -hx.z},
        {-hx.x, -hx.y, +hx.z}, {-hx.x, -hx.y, -hx.z}
    };

    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);

    for (int i = 0; i < 8; i++)
    {
        glm::vec3 worldPos = body.Position + rotMat * corners[i];
        min = glm::min(min, worldPos);
        max = glm::max(max, worldPos);
    }

    return { min, max };
}

static glm::mat3 ToMat3(const glm::quat& q)
{
    return glm::mat3_cast(q);
}

static void UpdateInertiaWorld(PhysicsBody& body)
{
    glm::mat3 R = ToMat3(body.Rotation);
    body.InverseInertiaWorld =
        R * body.InverseInertiaLocal * glm::transpose(R);
}

static glm::mat3 ComputeBoxInertia(const glm::vec3& halfExtents, float mass)
{
    float wx = 2.0f * halfExtents.x;
    float wy = 2.0f * halfExtents.y;
    float wz = 2.0f * halfExtents.z;

    float ix = (1.0f / 12.0f) * mass * (wy * wy + wz * wz);
    float iy = (1.0f / 12.0f) * mass * (wx * wx + wz * wz);
    float iz = (1.0f / 12.0f) * mass * (wx * wx + wy * wy);

    return glm::mat3(
        ix, 0, 0,
        0, iy, 0,
        0, 0, iz
    );
}

static glm::mat3 ComputeSphereInertia(float radius, float mass)
{
    float i = (2.0f / 5.0f) * mass * radius * radius;
    return glm::mat3(i);
}


PhysicsWorld::PhysicsWorld(Scene* scene)
    : m_Scene(scene)
{
}

PhysicsWorld::~PhysicsWorld()
{
    Shutdown();
}

void PhysicsWorld::Shutdown()
{
    m_Bodies.clear();
    m_Pairs.clear();
    m_Contacts.clear();
    m_Scene = nullptr;
}

void PhysicsWorld::Step(float dt)
{
    const int substeps = 2;
    float h = dt / substeps;

    for (int i = 0; i < substeps; ++i)
    {
        BuildBodies();
        IntegrateForces(h);
        Broadphase();
        Narrowphase();
        SolveContacts();
        IntegrateVelocities(h);
    }

    WriteBack();
}

void PhysicsWorld::BuildBodies()
{
    m_Bodies.clear();

    auto& registry = m_Scene->GetRegistry();

    auto view = registry.view<
        TransformComponent,
        RigidBodyComponent
    >();

    for (auto e : view)
    {
        auto& t = view.get<TransformComponent>(e);
        auto& rb = view.get<RigidBodyComponent>(e);

        PhysicsBody body;
        body.Entity = e;

        body.Position = t.Translation;
        body.Rotation = t.Rotation;

        body.Velocity = rb.Velocity;
        body.AngularVelocity = rb.AngularVelocity;

        body.IsStatic = rb.IsStatic;
        body.InverseMass = rb.IsStatic || rb.Mass <= 0.0f
            ? 0.0f
            : 1.0f / rb.Mass;

        body.Restitution = rb.Restitution;
        body.Friction = rb.Friction;

        if (registry.any_of<BoxColliderComponent>(e))
        {
            auto& box = registry.get<BoxColliderComponent>(e);
            body.HalfExtents = box.HalfExtents * t.Scale;
        }
        else if (registry.any_of<SphereColliderComponent>(e))
        {
            auto& s = registry.get<SphereColliderComponent>(e);
            body.HalfExtents = glm::vec3(s.Radius * glm::compMax(t.Scale));
        }

        if (body.IsStatic)
        {
            body.InertiaLocal = glm::mat3(0.0f);
            body.InverseInertiaLocal = glm::mat3(0.0f);
        }
        else
        {
            float mass = 1.0f / body.InverseMass;

            if (registry.any_of<BoxColliderComponent>(e))
            {
                body.InertiaLocal = ComputeBoxInertia(body.HalfExtents, mass);
            }
            else if (registry.any_of<SphereColliderComponent>(e))
            {
                float radius = body.HalfExtents.x;
                body.InertiaLocal = ComputeSphereInertia(radius, mass);
            }

            body.InverseInertiaLocal = glm::inverse(body.InertiaLocal);
        }

        UpdateInertiaWorld(body);

        m_Bodies.push_back(body);
    }
}

void PhysicsWorld::IntegrateForces(float dt)
{
    for (auto& body : m_Bodies)
    {
        if (body.IsStatic)
            continue;

        body.Velocity += m_Gravity * dt;
    }
}

static AABB ComputeAABB(const PhysicsBody& body)
{
    return {
        body.Position - body.HalfExtents,
        body.Position + body.HalfExtents
    };
}

static bool AABBOverlap(const AABB& a, const AABB& b)
{
    return (a.Min.x <= b.Max.x && a.Max.x >= b.Min.x) &&
        (a.Min.y <= b.Max.y && a.Max.y >= b.Min.y) &&
        (a.Min.z <= b.Max.z && a.Max.z >= b.Min.z);
}

void PhysicsWorld::Broadphase()
{
    m_Pairs.clear();

    for (size_t i = 0; i < m_Bodies.size(); i++)
    {
        for (size_t j = i + 1; j < m_Bodies.size(); j++)
        {
            auto& A = m_Bodies[i];
            auto& B = m_Bodies[j];

            if (A.IsStatic && B.IsStatic)
                continue;

            if (AABBOverlap(ComputeAABBFromOBB(A), ComputeAABBFromOBB(B)))
                m_Pairs.push_back({ &A, &B });
        }
    }
}

static bool BoxBoxSAT(
    PhysicsBody* A,
    PhysicsBody* B,
    ContactManifold& manifold)
{
    glm::vec3 delta = B->Position - A->Position;
    glm::vec3 overlap = (A->HalfExtents + B->HalfExtents) - glm::abs(delta);

    if (overlap.x < 0 || overlap.y < 0 || overlap.z < 0)
        return false;

    // Find smallest axis
    if (overlap.x < overlap.y && overlap.x < overlap.z)
        manifold.Normal = { glm::sign(delta.x), 0, 0 };
    else if (overlap.y < overlap.z)
        manifold.Normal = { 0, glm::sign(delta.y), 0 };
    else
        manifold.Normal = { 0, 0, glm::sign(delta.z) };

    manifold.A = A;
    manifold.B = B;
    manifold.ContactCount = 1;
    manifold.Contacts[0].Penetration = glm::compMin(overlap);
    manifold.Contacts[0].Position =
        A->Position + 0.5f * delta;

    return true;
}

static bool SphereSphere(
    PhysicsBody* A,
    PhysicsBody* B,
    ContactManifold& manifold)
{
    float rA = A->HalfExtents.x;
    float rB = B->HalfExtents.x;

    glm::vec3 delta = B->Position - A->Position;
    float dist2 = glm::dot(delta, delta);
    float r = rA + rB;

    if (dist2 > r * r)
        return false;

    float dist = sqrt(dist2);

    manifold.A = A;
    manifold.B = B;
    manifold.Normal = dist > 0 ? delta / dist : glm::vec3(0, 1, 0);
    manifold.ContactCount = 1;
    manifold.Contacts[0].Penetration = r - dist;
    manifold.Contacts[0].Position =
        A->Position + manifold.Normal * rA;

    return true;
}

static bool SAT_OBB(
    PhysicsBody* A,
    PhysicsBody* B,
    ContactManifold& manifold)
{
    // Rotation matrices
    glm::mat3 RA = glm::mat3_cast(A->Rotation);
    glm::mat3 RB = glm::mat3_cast(B->Rotation);

    // Box half extents
    glm::vec3 a = A->HalfExtents;
    glm::vec3 b = B->HalfExtents;

    // Rotation matrix expressing B in A's frame
    glm::mat3 R, AbsR;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            R[i][j] = glm::dot(RA[i], RB[j]);
            AbsR[i][j] = fabs(R[i][j]) + 1e-6f; // avoid divide by zero
        }
    }

    glm::vec3 t = B->Position - A->Position;
    t = glm::vec3(glm::dot(t, RA[0]), glm::dot(t, RA[1]), glm::dot(t, RA[2]));

    float ra, rb, overlap;
    float minOverlap = FLT_MAX;
    glm::vec3 smallestAxis;

    // Test axes L = A0, A1, A2
    for (int i = 0; i < 3; i++)
    {
        ra = a[i];
        rb = b.x * AbsR[i][0] + b.y * AbsR[i][1] + b.z * AbsR[i][2];
        overlap = ra + rb - fabs(t[i]);
        if (overlap < 0) return false;
        if (overlap < minOverlap) { minOverlap = overlap; smallestAxis = RA[i] * glm::sign(t[i]); }
    }

    // Test axes L = B0, B1, B2
    for (int i = 0; i < 3; i++)
    {
        ra = a.x * AbsR[0][i] + a.y * AbsR[1][i] + a.z * AbsR[2][i];
        rb = b[i];
        overlap = ra + rb - fabs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]);
        if (overlap < 0) return false;
        if (overlap < minOverlap) { minOverlap = overlap; smallestAxis = RB[i] * glm::sign(glm::dot(t, RB[i])); }
    }

    // TODO: Cross-product axes test (optional for initial simplicity)
    // For most cases with boxes, testing face normals suffices

    // Fill manifold
    manifold.A = A;
    manifold.B = B;
    manifold.Normal = glm::normalize(smallestAxis);
    manifold.ContactCount = 1;
    manifold.Contacts[0].Position = 0.5f * (A->Position + B->Position);
    manifold.Contacts[0].Penetration = minOverlap;

    return true;
}

void PhysicsWorld::Narrowphase()
{
    m_Contacts.clear();

    for (auto& pair : m_Pairs)
    {
        ContactManifold manifold{};

        if (SAT_OBB(pair.A, pair.B, manifold) ||
            SphereSphere(pair.A, pair.B, manifold))
        {
            m_Contacts.push_back(manifold);
        }
    }
}

void PhysicsWorld::SolveContacts()
{
    const int iterations = 8;
    constexpr float slop = 0.01f;          // penetration allowance
    constexpr float percent = 0.8f;        // positional correction factor
    constexpr float baumgarte = 0.2f;      // velocity bias factor

    for (auto& m : m_Contacts)
        m.AccumulatedImpulse = 0.0f; // reset warm-start

    for (int it = 0; it < iterations; it++)
    {
        for (auto& m : m_Contacts)
        {
            PhysicsBody* A = m.A;
            PhysicsBody* B = m.B;

            glm::vec3 contactPoint = m.Contacts[0].Position;
            glm::vec3 ra = contactPoint - A->Position;
            glm::vec3 rb = contactPoint - B->Position;

            // Relative velocity at contact
            glm::vec3 velA = A->Velocity + glm::cross(A->AngularVelocity, ra);
            glm::vec3 velB = B->Velocity + glm::cross(B->AngularVelocity, rb);
            glm::vec3 rv = velB - velA;

            glm::vec3 n = m.Normal;

            // Baumgarte velocity bias
            float penetration = glm::max(m.Contacts[0].Penetration - slop, 0.0f);
            float bias = (baumgarte * penetration) / (1.0f / 60.0f); // assuming fixed dt ~1/60s
            float velAlongNormal = glm::dot(rv, n) + bias;

            if (velAlongNormal > 0.0f)
                continue;

            // Compute normal impulse with inertia
            float denom =
                A->InverseMass + B->InverseMass +
                glm::dot(
                    glm::cross(A->InverseInertiaWorld * glm::cross(ra, n), ra) +
                    glm::cross(B->InverseInertiaWorld * glm::cross(rb, n), rb),
                    n
                );

            float e = glm::min(A->Restitution, B->Restitution);
            float j = -(1.0f + e) * velAlongNormal / denom;

            // Warm-starting
            float oldImpulse = m.AccumulatedImpulse;
            m.AccumulatedImpulse = glm::max(oldImpulse + j, 0.0f);
            j = m.AccumulatedImpulse - oldImpulse;

            glm::vec3 impulse = j * n;

            if (!A->IsStatic)
            {
                A->Velocity -= impulse * A->InverseMass;
                A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(ra, impulse);
            }

            if (!B->IsStatic)
            {
                B->Velocity += impulse * B->InverseMass;
                B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rb, impulse);
            }

            // Friction impulse
            velA = A->Velocity + glm::cross(A->AngularVelocity, ra);
            velB = B->Velocity + glm::cross(B->AngularVelocity, rb);
            rv = velB - velA;

            glm::vec3 tangent = rv - glm::dot(rv, n) * n;
            if (glm::length2(tangent) > 0.0001f)
                tangent = glm::normalize(tangent);

            float jt = -glm::dot(rv, tangent) / denom;
            float mu = sqrt(A->Friction * B->Friction);

            glm::vec3 frictionImpulse;
            if (fabs(jt) < j * mu)
                frictionImpulse = jt * tangent;
            else
                frictionImpulse = -j * mu * tangent;

            if (!A->IsStatic)
            {
                A->Velocity -= frictionImpulse * A->InverseMass;
                A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(ra, frictionImpulse);
            }

            if (!B->IsStatic)
            {
                B->Velocity += frictionImpulse * B->InverseMass;
                B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rb, frictionImpulse);
            }

            // Positional correction
            float posCorrection = percent * penetration / (A->InverseMass + B->InverseMass);
            glm::vec3 correction = posCorrection * n;

            if (!A->IsStatic) A->Position -= correction * A->InverseMass;
            if (!B->IsStatic) B->Position += correction * B->InverseMass;
        }
    }
}

void PhysicsWorld::IntegrateVelocities(float dt)
{
    constexpr float sleepLinear = 0.05f;
    constexpr float sleepAngular = 0.05f;

    for (auto& body : m_Bodies)
    {
        if (body.IsStatic)
            continue;

        // Linear integration
        body.Position += body.Velocity * dt;

        // Angular integration
        if (glm::length(body.AngularVelocity) > 0.0001f)
        {
            glm::quat dq = glm::quat(0.0f, body.AngularVelocity * dt) * body.Rotation * 0.5f;
            body.Rotation += dq;
            body.Rotation = glm::normalize(body.Rotation);
        }

        // Update inertia world
        UpdateInertiaWorld(body);

        // Sleep logic
        if (glm::length(body.Velocity) < sleepLinear &&
            glm::length(body.AngularVelocity) < sleepAngular)
        {
            body.Velocity = glm::vec3(0.0f);
            body.AngularVelocity = glm::vec3(0.0f);
        }
    }
}

void PhysicsWorld::WriteBack()
{
    auto& registry = m_Scene->GetRegistry();

    for (auto& body : m_Bodies)
    {
        auto& t = registry.get<TransformComponent>(body.Entity);
        auto& rb = registry.get<RigidBodyComponent>(body.Entity);

        t.Translation = body.Position;
        t.Rotation = body.Rotation;

        rb.Velocity = body.Velocity;
        rb.AngularVelocity = body.AngularVelocity;
    }
}
