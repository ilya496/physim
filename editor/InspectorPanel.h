#pragma once

#include <memory>
#include "scene/Scene.h"
#include "scene/SceneController.h"

class InspectorPanel
{
public:
    InspectorPanel(SceneController* sceneController);

    void Draw(std::shared_ptr<Scene> scene);

private:
    void DrawEntityInspector(std::shared_ptr<Scene>, entt::entity entity);

    template<typename T>
    void DrawComponent(const char* name, std::shared_ptr<Scene>, entt::entity entity);

    void DrawAddComponentPopup(std::shared_ptr<Scene>, entt::entity entity);
    void DrawEntityPicker(std::shared_ptr<Scene> scene,
        entt::entity& target,
        const char* label);

private:
    SceneController* m_SceneController = nullptr;
    entt::entity m_PendingJointEntity = entt::null;

    bool m_CreateDistanceJoint = false;
    bool m_OpenJointTargetPopup = false;
};