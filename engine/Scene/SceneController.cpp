#include "SceneController.h"

#include "Components.h"
#include "project/Project.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

SceneController::SceneController()
{
}

// ------------------------------------------------------------
// Scene Setup
// ------------------------------------------------------------

void SceneController::SetEditorScene(const std::shared_ptr<Scene>& scene)
{
    Stop(); // ensure clean state
    m_EditorScene = scene;
}

// ------------------------------------------------------------
// Playback Controls
// ------------------------------------------------------------

void SceneController::Play()
{
    if (!m_EditorScene)
        return;

    if (m_State == SimulationState::Stopped)
    {
        // 1. Clone editor scene
        m_RuntimeScene = m_EditorScene->Copy();

        // 2. Create physics world
        m_PhysicsWorld = std::make_unique<PhysicsWorld>();

        // 3. Initialize physics bodies
        InitializePhysicsFromScene();

        // 4. Reset timeline
        ClearHistory();
        m_Accumulator = 0.0f;
    }

    Project::GetActive()->SetActiveScene(m_RuntimeScene);
    m_State = SimulationState::Running;
}

void SceneController::Pause()
{
    if (m_State == SimulationState::Running)
        m_State = SimulationState::Paused;
}

void SceneController::Stop()
{
    m_State = SimulationState::Stopped;

    m_PhysicsWorld.reset();
    m_RuntimeScene.reset();

    ClearHistory();

    Project::GetActive()->SetActiveScene(m_EditorScene);
}

// ------------------------------------------------------------
// Update Loop (Fixed Timestep)
// ------------------------------------------------------------

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

// ------------------------------------------------------------
// Physics Initialization
// ------------------------------------------------------------

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
            mass
        );

        body->Orientation = tr.Rotation;
        body->Restitution = rb.Restitution;
        body->Friction = rb.Friction;
        body->ID = static_cast<uint32_t>(entity);

        rb.RuntimeBody = body;
    }
}

// ------------------------------------------------------------
// Timeline Recording
// ------------------------------------------------------------

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
    m_CurrentFrameIndex = static_cast<int>(m_History.size()) - 1;

    // Limit memory usage
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

// ------------------------------------------------------------
// Timeline Navigation
// ------------------------------------------------------------

void SceneController::SetFrame(int frameIndex)
{
    if (m_State == SimulationState::Stopped)
        return;

    if (frameIndex < 0 || frameIndex >= static_cast<int>(m_History.size()))
        return;

    m_State = SimulationState::Paused;
    m_CurrentFrameIndex = frameIndex;

    if (m_PhysicsWorld)
        m_PhysicsWorld->SetState(m_History[m_CurrentFrameIndex]);

    SyncSceneToPhysics();
}

void SceneController::StepFrame(int direction)
{
    SetFrame(m_CurrentFrameIndex + direction);
}

// ------------------------------------------------------------
// Scene Synchronization
// ------------------------------------------------------------

void SceneController::SyncSceneToPhysics()
{
    if (!m_RuntimeScene || m_History.empty())
        return;

    const auto& snapshot = m_History[m_CurrentFrameIndex];

    for (const auto& [entityID, state] : snapshot)
    {
        auto& registry = m_RuntimeScene->GetRegistry();
        auto entity = static_cast<entt::entity>(entityID);
        auto& tr = registry.get<TransformComponent>(entity);

        tr.Translation = state.Position;
        tr.Rotation = state.Orientation;
    }
}
