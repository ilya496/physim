#pragma once

#include <memory>
#include "render/Model.h"
#include "scene/Entity.h"
#include "scene/Scene.h"
#include "imgui.h"

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel();

    void Draw(std::shared_ptr<Scene> scene);

private:
    void DrawEntityNode(Entity entity);

    ImTextureID GetEntityIcon(Entity entity);

    std::shared_ptr<Texture> m_MeshIcon;
    std::shared_ptr<Texture> m_LightIcon;
    std::shared_ptr<Texture> m_EmptyIcon;
};