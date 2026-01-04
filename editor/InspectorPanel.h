#pragma once

#include <memory>
#include "scene/Scene.h"

class InspectorPanel
{
public:
    void Draw(std::shared_ptr<Scene> scene);

private:
    void DrawEntityInspector(std::shared_ptr<Scene>, entt::entity entity);

    template<typename T>
    void DrawComponent(const char* name, std::shared_ptr<Scene>, entt::entity entity);

    void DrawAddComponentPopup(std::shared_ptr<Scene>, entt::entity entity);
};