#pragma once

#include <memory>
#include <deque>

#include "Scene.h"
#include "physics/PhysicsWorld.h"

// ------------------------------------------------------------
// SceneController
// ------------------------------------------------------------
// Responsible for:
// - Creating a runtime clone of the editor scene
// - Initializing physics bodies from ECS components
// - Running fixed-timestep simulation
// - Recording physics state history
// - Allowing frame scrubbing (timeline navigation)
// - Synchronizing physics state back to ECS transforms
// ------------------------------------------------------------

enum class SimulationState
{
    Stopped = 0,
    Running,
    Paused
};

class SceneController
{
public:

    SceneController();
    ~SceneController() = default;

    void SetEditorScene(const std::shared_ptr<Scene>& scene);

    const std::shared_ptr<Scene>& GetRuntimeScene() const { return m_RuntimeScene; }
    SimulationState GetState() const { return m_State; }

    void Play();
    void Pause();
    void TogglePause();
    void Stop();

    void Update(float dt);

    void SetFrame(int frameIndex);
    void StepFrame(int direction);

    int GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
    int GetTotalFrames() const { return static_cast<int>(m_History.size()); }

    // Editor-side creation
    void CreateDistanceJoint(entt::entity a, entt::entity b,
        const glm::vec3& localAnchorA,
        const glm::vec3& localAnchorB);

    float GetFixedDeltaTime() const { return m_FixedDeltaTime; }

private:
    void InitializePhysicsFromScene();
    void RecordFrame();
    void SyncSceneToPhysics();
    void ClearHistory();

private:

    std::shared_ptr<Scene> m_EditorScene;
    std::shared_ptr<Scene> m_RuntimeScene;

    std::unique_ptr<PhysicsWorld> m_PhysicsWorld;

    SimulationState m_State = SimulationState::Stopped;

    float m_Accumulator = 0.0f;
    const float m_FixedDeltaTime = 1.0f / 60.0f;

    std::deque<PhysicsSnapshot> m_History;
    int m_CurrentFrameIndex = 0;

    static constexpr size_t s_MaxHistoryFrames = 3600; // 1 minute @ 60 FPS
};
