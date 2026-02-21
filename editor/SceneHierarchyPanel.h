#pragma once

#include <memory>
#include "render/Model.h"
#include "scene/Entity.h"
#include "scene/Scene.h"
#include "imgui.h"

class SceneHierarchyPanel
{
public:
    void Draw(std::shared_ptr<Scene> scene);

private:
    bool DrawEntityNode(Entity entity);
};