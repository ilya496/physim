#include "PhysicsWorld.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>

static AABB ComputeWorldAABB(
    const Shape* shape,
    const glm::vec3& position,
    const glm::quat& rotation)
{
    const AABB& local = shape->LocalAABB;

    glm::vec3 corners[8] =
    {
        { local.Min.x, local.Min.y, local.Min.z },
        { local.Max.x, local.Min.y, local.Min.z },
        { local.Min.x, local.Max.y, local.Min.z },
        { local.Max.x, local.Max.y, local.Min.z },
        { local.Min.x, local.Min.y, local.Max.z },
        { local.Max.x, local.Min.y, local.Max.z },
        { local.Min.x, local.Max.y, local.Max.z },
        { local.Max.x, local.Max.y, local.Max.z }
    };

    glm::vec3 min(FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    for (auto& c : corners)
    {
        glm::vec3 world = position + rotation * c;
        min = glm::min(min, world);
        max = glm::max(max, world);
    }

    return { min, max };
}


static glm::mat3 ComputeWorldInertia(
    const glm::mat3& localInvInertia,
    const glm::quat& rotation)
{
    glm::mat3 R = glm::mat3_cast(rotation);
    return R * localInvInertia * glm::transpose(R);
}


int32_t DynamicAABBTree::AllocateNode()
{
    if (m_FreeList != -1)
    {
        int32_t node = m_FreeList;
        m_FreeList = m_Nodes[node].Parent;
        m_Nodes[node] = {};
        return node;
    }

    m_Nodes.emplace_back();
    return static_cast<int32_t>(m_Nodes.size() - 1);
}

int32_t DynamicAABBTree::CreateProxy(const AABB& aabb, void* userData)
{
    int32_t node = AllocateNode();

    m_Nodes[node].Box = aabb;
    m_Nodes[node].UserData = userData;
    m_Nodes[node].Height = 0;

    InsertLeaf(node);
    return node;
}

void DynamicAABBTree::InsertLeaf(int32_t leaf)
{
    if (m_Root == -1)
    {
        m_Root = leaf;
        m_Nodes[m_Root].Parent = -1;
        return;
    }

    int32_t index = m_Root;
    while (!m_Nodes[index].IsLeaf())
    {
        int32_t left = m_Nodes[index].Left;
        int32_t right = m_Nodes[index].Right;

        float costLeft = glm::length(m_Nodes[left].Box.Max - m_Nodes[left].Box.Min);
        float costRight = glm::length(m_Nodes[right].Box.Max - m_Nodes[right].Box.Min);

        index = (costLeft < costRight) ? left : right;
    }

    int32_t sibling = index;
    int32_t parent = AllocateNode();

    m_Nodes[parent].Parent = m_Nodes[sibling].Parent;
    m_Nodes[parent].Left = sibling;
    m_Nodes[parent].Right = leaf;
    m_Nodes[parent].Box.Min = glm::min(m_Nodes[sibling].Box.Min, m_Nodes[leaf].Box.Min);
    m_Nodes[parent].Box.Max = glm::max(m_Nodes[sibling].Box.Max, m_Nodes[leaf].Box.Max);

    m_Nodes[sibling].Parent = parent;
    m_Nodes[leaf].Parent = parent;

    if (m_Nodes[parent].Parent == -1)
        m_Root = parent;
}

void DynamicAABBTree::ComputePairs(std::vector<BroadPhasePair>& outPairs)
{
    if (m_Root == -1)
        return;

    for (size_t i = 0; i < m_Nodes.size(); ++i)
    {
        if (!m_Nodes[i].IsLeaf())
            continue;

        for (size_t j = i + 1; j < m_Nodes.size(); ++j)
        {
            if (!m_Nodes[j].IsLeaf())
                continue;

            if (m_Nodes[i].Box.Overlaps(m_Nodes[j].Box))
            {
                outPairs.push_back({
                    static_cast<int32_t>(i),
                    static_cast<int32_t>(j)
                    });
            }
        }
    }
}

// TODO: implement later
void DynamicAABBTree::DestroyProxy(int32_t proxyId)
{
    DeleteLeaf(proxyId);
    FreeNode(proxyId);
}

void DynamicAABBTree::MoveProxy(int32_t proxyId, const AABB& aabb)
{
    DeleteLeaf(proxyId);
    m_Nodes[proxyId].Box = aabb;
    InsertLeaf(proxyId);
}

void DynamicAABBTree::DeleteLeaf(int32_t leaf)
{
    if (leaf == m_Root)
    {
        m_Root = -1;
        return;
    }

    int32_t parent = m_Nodes[leaf].Parent;
    int32_t grandParent = m_Nodes[parent].Parent;
    int32_t sibling =
        (m_Nodes[parent].Left == leaf)
        ? m_Nodes[parent].Right
        : m_Nodes[parent].Left;

    if (grandParent != -1)
    {
        if (m_Nodes[grandParent].Left == parent)
            m_Nodes[grandParent].Left = sibling;
        else
            m_Nodes[grandParent].Right = sibling;

        m_Nodes[sibling].Parent = grandParent;
        FreeNode(parent);
    }
    else
    {
        m_Root = sibling;
        m_Nodes[sibling].Parent = -1;
        FreeNode(parent);
    }
}

void DynamicAABBTree::FreeNode(int32_t node)
{
    m_Nodes[node].Parent = m_FreeList;
    m_FreeList = node;
}

int32_t DynamicAABBTree::Balance(int32_t node)
{
    return 1;
}

void ContactSolver::Solve(
    std::vector<ContactManifold>& manifolds,
    float dt,
    int iterations)
{
    std::vector<ContactConstraint> constraints;

    for (auto& m : manifolds)
    {
        for (uint32_t i = 0; i < m.ContactCount; ++i)
        {
            ContactConstraint c{};
            c.A = m.BodyA;
            c.B = m.BodyB;
            c.Normal = glm::normalize(m.Normal);
            c.ContactPoint = m.Contacts[i].Position;

            c.rA = c.ContactPoint - c.A->Position;
            c.rB = c.ContactPoint - c.B->Position;

            glm::mat3 invIA = ComputeWorldInertia(
                c.A->InverseInertiaLocal, c.A->Rotation);
            glm::mat3 invIB = ComputeWorldInertia(
                c.B->InverseInertiaLocal, c.B->Rotation);

            glm::vec3 rnA = glm::cross(c.rA, c.Normal);
            glm::vec3 rnB = glm::cross(c.rB, c.Normal);

            float kNormal =
                c.A->InverseMass +
                c.B->InverseMass +
                glm::dot(glm::cross(invIA * rnA, c.rA), c.Normal) +
                glm::dot(glm::cross(invIB * rnB, c.rB), c.Normal);

            c.NormalMass = kNormal > 0.0f ? 1.0f / kNormal : 0.0f;

            float penetration = m.Contacts[i].Penetration;
            c.Bias = -0.2f * std::min(0.0f, penetration) / dt;

            constraints.push_back(c);
        }
    }

    for (int it = 0; it < iterations; ++it)
    {
        for (auto& c : constraints)
        {
            glm::vec3 vA = c.A->LinearVelocity +
                glm::cross(c.A->AngularVelocity, c.rA);
            glm::vec3 vB = c.B->LinearVelocity +
                glm::cross(c.B->AngularVelocity, c.rB);

            glm::vec3 dv = vB - vA;
            float vn = glm::dot(dv, c.Normal);

            float lambda = c.NormalMass * (-vn + c.Bias);
            float oldImpulse = c.AccumulatedNormalImpulse;

            c.AccumulatedNormalImpulse =
                glm::max(oldImpulse + lambda, 0.0f);
            lambda = c.AccumulatedNormalImpulse - oldImpulse;

            glm::vec3 impulse = lambda * c.Normal;

            c.A->LinearVelocity -= impulse * c.A->InverseMass;
            c.B->LinearVelocity += impulse * c.B->InverseMass;

            c.A->AngularVelocity -=
                ComputeWorldInertia(
                    c.A->InverseInertiaLocal, c.A->Rotation) *
                glm::cross(c.rA, impulse);

            c.B->AngularVelocity +=
                ComputeWorldInertia(
                    c.B->InverseInertiaLocal, c.B->Rotation) *
                glm::cross(c.rB, impulse);
        }
    }
}

inline bool SphereSphere(
    const SphereShape* a,
    const SphereShape* b,
    const glm::vec3& posA,
    const glm::vec3& posB,
    ContactManifold& manifold
)
{
    glm::vec3 d = posB - posA;
    float dist2 = glm::length2(d);
    float r = a->Radius + b->Radius;

    if (dist2 > r * r)
        return false;

    float dist = sqrt(dist2);
    manifold.Normal = dist > 0.0f ? d / dist : glm::vec3(0.0f, 1.0f, 0.0f);
    manifold.Contacts[0].Position = posA + manifold.Normal * a->Radius;
    manifold.Contacts[0].Penetration = r - dist;
    manifold.ContactCount = 1;

    return true;
}

bool BoxBox(
    const BoxShape*,
    const BoxShape*,
    const AABB& a,
    const AABB& b,
    ContactManifold& manifold)
{
    if (!a.Overlaps(b))
        return false;

    glm::vec3 centerA = (a.Min + a.Max) * 0.5f;
    glm::vec3 centerB = (b.Min + b.Max) * 0.5f;

    glm::vec3 d = centerB - centerA;
    manifold.Normal = glm::normalize(d);
    manifold.Contacts[0].Position = (centerA + centerB) * 0.5f;
    manifold.Contacts[0].Penetration = glm::length(d);
    manifold.ContactCount = 1;

    return true;
}

bool NarrowPhase::GenerateContacts(
    const Shape* shapeA,
    const Shape* shapeB,
    ContactManifold& manifold)
{
    if (shapeA->Type == ShapeType::Sphere &&
        shapeB->Type == ShapeType::Sphere)
    {
        return SphereSphere(
            static_cast<const SphereShape*>(shapeA),
            static_cast<const SphereShape*>(shapeB),
            manifold.BodyA->Position,
            manifold.BodyB->Position,
            manifold);
    }

    return false;
}

void PhysicsWorld::Step(float dt)
{
    m_ContactManifolds.clear();

    for (auto& body : m_RigidBodies)
    {
        if (body.Type == BodyType::Dynamic)
            body.Force += m_Gravity / body.InverseMass;
    }

    for (auto& body : m_RigidBodies)
        m_Integrator.Integrate(body, dt);

    std::vector<BroadPhasePair> pairs;
    m_BroadPhase->ComputePairs(pairs);

    for (auto& p : pairs)
    {
        Collider& cA = m_Colliders[p.ProxyA];
        Collider& cB = m_Colliders[p.ProxyB];

        ContactManifold m{};
        m.BodyA = &m_RigidBodies[p.ProxyA];
        m.BodyB = &m_RigidBodies[p.ProxyB];

        if (m_NarrowPhase.GenerateContacts(
            cA.ColissionShape,
            cB.ColissionShape,
            m))
        {
            m_ContactManifolds.push_back(m);
        }
    }

    ContactSolver solver;
    solver.Solve(m_ContactManifolds, dt);
}


PhysicsWorld::~PhysicsWorld()
{
}