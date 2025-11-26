#pragma once

#include <string>

#include "Entity.h"
#include "UUID.h"

class Scene
{
public:
    Scene();
    ~Scene();

    Entity CreateEntity(const std::string& name = std::string());
    Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
    void DestroyEntity(Entity entity);

private:
    entt::registry m_Registry;
};
