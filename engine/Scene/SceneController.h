#pragma once

#include <memory>
#include <deque>
#include <unordered_map>

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

    // --------------------------------------------------------
    // Scene Assignment
    // --------------------------------------------------------
    void SetEditorScene(const std::shared_ptr<Scene>& scene);

    const std::shared_ptr<Scene>& GetRuntimeScene() const { return m_RuntimeScene; }
    SimulationState GetState() const { return m_State; }

    // --------------------------------------------------------
    // Playback Controls
    // --------------------------------------------------------
    void Play();
    void Pause();
    void Stop();

    void Update(float dt);

    // --------------------------------------------------------
    // Timeline Controls
    // --------------------------------------------------------
    void SetFrame(int frameIndex);
    void StepFrame(int direction);

    int GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
    int GetTotalFrames() const { return static_cast<int>(m_History.size()); }

private:

    void InitializePhysicsFromScene();
    void RecordFrame();
    void SyncSceneToPhysics();
    void ClearHistory();

private:

    // --------------------------------------------------------
    // Scene References
    // --------------------------------------------------------
    std::shared_ptr<Scene> m_EditorScene;
    std::shared_ptr<Scene> m_RuntimeScene;

    // --------------------------------------------------------
    // Physics
    // --------------------------------------------------------
    std::unique_ptr<PhysicsWorld> m_PhysicsWorld;

    // --------------------------------------------------------
    // Simulation State
    // --------------------------------------------------------
    SimulationState m_State = SimulationState::Stopped;

    float m_Accumulator = 0.0f;
    const float m_FixedDeltaTime = 1.0f / 60.0f;

    // --------------------------------------------------------
    // Timeline / History
    // --------------------------------------------------------
    std::deque<PhysicsSnapshot> m_History;
    int m_CurrentFrameIndex = 0;

    static constexpr size_t s_MaxHistoryFrames = 3600; // 1 minute @ 60 FPS
};
