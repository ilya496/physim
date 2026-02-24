#include "SceneController.h"

#include "Components.h"
#include "project/Project.h"

#include <iostream>

SceneController::SceneController()
{
}

void SceneController::SetEditorScene(const std::shared_ptr<Scene>& scene)
{
    Stop(); // ensure clean state
    m_EditorScene = scene;
}

void SceneController::Play()
{
    if (!m_EditorScene)
        return;

    if (m_State == SimulationState::Stopped)
    {
        m_RuntimeScene = m_EditorScene->Copy();
        m_PhysicsWorld = std::make_unique<PhysicsWorld>();

        InitializePhysicsFromScene();
        ClearHistory();
        m_Accumulator = 0.0f;

        Project::GetActive()->SetActiveScene(m_RuntimeScene);
    }

    m_State = SimulationState::Running;
}

void SceneController::Pause()
{
    if (m_State == SimulationState::Running)
        m_State = SimulationState::Paused;
}

void SceneController::TogglePause()
{
    if (m_State == SimulationState::Running)
        m_State = SimulationState::Paused;
    else if (m_State == SimulationState::Paused)
        m_State = SimulationState::Running;
}

void SceneController::Stop()
{
    if (m_State == SimulationState::Stopped)
        return;

    m_State = SimulationState::Stopped;

    m_PhysicsWorld.reset();
    m_RuntimeScene.reset();

    ClearHistory();

    if (m_EditorScene)
        Project::GetActive()->SetActiveScene(m_EditorScene);
}

void SceneController::Update(float dt)
{
    if (m_State != SimulationState::Running)
        return;

    if (!m_PhysicsWorld)
        return;

    m_Accumulator += dt;

    while (m_Accumulator >= m_FixedDeltaTime)
    {
        m_PhysicsWorld->Step(m_FixedDeltaTime);

        RecordFrame();

        m_Accumulator -= m_FixedDeltaTime;
    }

    SyncSceneToPhysics();
}

void SceneController::InitializePhysicsFromScene()
{
    auto view = m_RuntimeScene->GetRegistry()
        .view<RigidBodyComponent, TransformComponent>();

    for (auto [entity, rb, tr] : view.each())
    {
        Shape* shape = nullptr;

        if (m_RuntimeScene->HasComponent<BoxColliderComponent>(entity))
        {
            auto& box = m_RuntimeScene->GetComponent<BoxColliderComponent>(entity);
            shape = new BoxShape(box.HalfExtents);
        }
        else if (m_RuntimeScene->HasComponent<SphereColliderComponent>(entity))
        {
            auto& sphere = m_RuntimeScene->GetComponent<SphereColliderComponent>(entity);
            shape = new SphereShape(sphere.Radius);
        }

        if (!shape)
            continue;

        float mass = rb.IsStatic ? 0.0f : rb.Mass;

        RigidBody* body = m_PhysicsWorld->CreateBody(
            tr.Translation,
            shape,
            rb.IsStatic ? BodyType::Static : BodyType::Dynamic,
            mass
        );

        body->Orientation = tr.Rotation;
        body->Material.Restitution = rb.Restitution;
        body->Material.Friction = rb.Friction;
        body->ID = static_cast<uint32_t>(entity);

        rb.RuntimeBody = body;
    }

    // ----------- Create Runtime Constraints -----------

    auto& registry = m_RuntimeScene->GetRegistry();

    // Distance joints
    // {
    auto viewDJ = registry.view<DistanceJointComponent, RigidBodyComponent>();

    for (auto [entity, joint, rbA] : viewDJ.each())
    {
        auto* rbB = registry.try_get<RigidBodyComponent>(joint.ConnectedEntity);
        if (!rbB)
            continue;

        if (!rbA.RuntimeBody || !rbB->RuntimeBody)
            continue;

        DistanceJoint* j = m_PhysicsWorld->AddDistanceJoint(
            (RigidBody*)rbA.RuntimeBody,
            (RigidBody*)rbB->RuntimeBody,
            joint.LocalAnchorA,
            joint.LocalAnchorB
        );

        j->TargetLength = joint.TargetLength;
    }
}

void SceneController::RecordFrame()
{
    PhysicsSnapshot snapshot;

    for (auto* body : m_PhysicsWorld->Bodies)
    {
        snapshot[body->ID] = {
            body->Position,
            body->Orientation,
            body->LinearVelocity,
            body->AngularVelocity
        };
    }

    m_History.push_back(std::move(snapshot));
    m_CurrentFrameIndex = (int)m_History.size() - 1;

    if (m_History.size() > s_MaxHistoryFrames)
    {
        m_History.pop_front();
        m_CurrentFrameIndex--;
    }
}

void SceneController::ClearHistory()
{
    m_History.clear();
    m_CurrentFrameIndex = 0;
}

void SceneController::SetFrame(int frameIndex)
{
    if (m_State == SimulationState::Stopped)
        return;

    if (frameIndex < 0 || frameIndex >= (int)m_History.size())
        return;

    m_State = SimulationState::Paused;
    m_CurrentFrameIndex = frameIndex;

    if (m_PhysicsWorld)
        m_PhysicsWorld->SetState(m_History[m_CurrentFrameIndex]);

    SyncSceneToPhysics();
}

void SceneController::StepFrame(int direction)
{
    if (m_State == SimulationState::Stopped || !m_PhysicsWorld)
        return;

    int target = m_CurrentFrameIndex + direction;

    // step backwards
    if (direction < 0)
    {
        if (target >= 0)
            SetFrame(target);
    }

    // step forward
    if (direction > 0)
    {
        // next frame already recorded
        if (target < (int)m_History.size())
        {
            SetFrame(target);
        }
        else
        {
            // at the end -> simulate next step
            m_State = SimulationState::Paused;
            m_PhysicsWorld->Step(m_FixedDeltaTime);
            RecordFrame();

            SetFrame(m_CurrentFrameIndex);
        }
    }
}

void SceneController::SyncSceneToPhysics()
{
    if (!m_RuntimeScene || m_History.empty())
        return;

    auto& registry = m_RuntimeScene->GetRegistry();
    const auto& snapshot = m_History[m_CurrentFrameIndex];

    for (const auto& [entityID, state] : snapshot)
    {
        entt::entity entity = (entt::entity)entityID;

        if (!registry.valid(entity) || !registry.all_of<TransformComponent>(entity))
            continue;

        auto& tr = registry.get<TransformComponent>(entity);
        tr.Translation = state.Position;
        tr.Rotation = state.Orientation;
    }
}

void SceneController::CreateDistanceJoint(entt::entity a,
    entt::entity b,
    const glm::vec3& localAnchorA,
    const glm::vec3& localAnchorB)
{
    if (!m_EditorScene)
        return;

    auto& registry = m_EditorScene->GetRegistry();

    if (!registry.valid(a) || !registry.valid(b))
        return;

    auto& comp = registry.emplace<DistanceJointComponent>(a);
    comp.ConnectedEntity = b;
    comp.LocalAnchorA = localAnchorA;
    comp.LocalAnchorB = localAnchorB;

    auto& trA = registry.get<TransformComponent>(a);
    auto& trB = registry.get<TransformComponent>(b);

    glm::vec3 worldA = trA.Translation + trA.Rotation * localAnchorA;
    glm::vec3 worldB = trB.Translation + trB.Rotation * localAnchorB;

    comp.TargetLength = glm::length(worldA - worldB);
}