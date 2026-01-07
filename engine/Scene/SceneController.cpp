#include "SceneController.h"

#include "project/Project.h"

void SceneController::Play()
{
    if (m_State == SimulationState::Running)
        return;

    m_RuntimeScene = m_EditorScene->Copy();

    // m_PhysicsWorld = std::make_unique<PhysicsWorld>(m_RuntimeScene.get());

    Project::GetActive()->SetActiveScene(m_RuntimeScene);

    m_State = SimulationState::Running;
}

void SceneController::Pause()
{
    if (m_State != SimulationState::Running)
        return;

    m_State = SimulationState::Paused;
}

void SceneController::Stop()
{
    if (m_State == SimulationState::Stopped)
        return;

    // m_PhysicsWorld->Shutdown();
    // m_PhysicsWorld.reset();
    m_RuntimeScene.reset();

    Project::GetActive()->SetActiveScene(m_EditorScene);

    m_State = SimulationState::Stopped;
}

void SceneController::SetEditorScene(const std::shared_ptr<Scene>& scene)
{
    m_EditorScene = scene;
}

void SceneController::Update(float dt)
{
    if (m_State != SimulationState::Running)
        return;

    constexpr float fixedStep = 1.0f / 60.0f;
    static float accumulator = 0.0f;

    accumulator += dt;
    while (accumulator >= fixedStep)
    {
        // m_PhysicsWorld->Step(fixedStep);
        accumulator -= fixedStep;
    }
}