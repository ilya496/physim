#include "PhysicsWorld.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>

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

}

void DynamicAABBTree::MoveProxy(int32_t proxyId, const AABB& aabb)
{

}

void DynamicAABBTree::FreeNode(int32_t node)
{

}

int32_t DynamicAABBTree::Balance(int32_t node)
{

}

void ContactSolver::Solve(
    std::vector<ContactManifold>& manifolds,
    float dt,
    int iterations
)
{
    std::vector<ContactContraint> constraints;

    for (auto& m : manifolds)
    {
        for (uint32_t i = 0; i < m.ContactCount; ++i)
        {
            ContactContraint c{};
            c.BodyA = m.BodyA;
            c.BodyB = m.BodyB;
            c.Normal = m.Normal;
            c.ContactPoint = m.Contacts[i].Position;
            c.Penetration = m.Contacts[i].Penetration;

            float invMassSum = c.BodyA->InverseMass = c.BodyB->InverseMass;
            c.NormalMass = invMassSum > 0.0f ? 1 / invMassSum : 0.0f;

            c.Bias = -0.2f * std::min(0.0f, c.Penetration) / dt;

            constraints.push_back(c);
        }
    }

    for (int it = 0; it < iterations; +it)
    {
        for (auto& c : constraints)
        {
            glm::vec3 relVel =
                c.BodyB->LinearVelocity - c.BodyA->LinearVelocity;

            float vn = glm::dot(relVel, c.Normal);
            float lambda = c.NormalMass * (-vn + c.Bias);

            float oldImpulse = c.AccumulatedImpulse;
            c.AccumulatedImpulse = glm::max(oldImpulse + lambda, 0.0f);
            lambda = c.AccumulatedImpulse - oldImpulse;

            glm::vec3 impulse = lambda * c.Normal;

            c.BodyA->LinearVelocity -= impulse * c.BodyA->InverseMass;
            c.BodyB->LinearVelocity += impulse * c.BodyB->InverseMass;
        }
    }
}