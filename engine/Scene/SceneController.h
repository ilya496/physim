#pragma once

#include "Scene.h"
#include "physics/PhysicsWorld.h"

enum class SimulationState
{
    Running,
    Paused,
    Stopped
};

class SceneController
{
public:
    void Play();
    void Pause();
    void Stop();

    void Update(float dt);
    void SetEditorScene(const std::shared_ptr<Scene>& scene);

private:
    SimulationState m_State = SimulationState::Stopped;

    std::shared_ptr<Scene> m_EditorScene;
    std::shared_ptr<Scene> m_RuntimeScene;

    std::unique_ptr<PhysicsWorld> m_PhysicsWorld;
};
