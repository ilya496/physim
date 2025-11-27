#pragma once

#include <string>
#include "entt.hpp"

#include "Components.h"
#include "UUID.h"

class Entity;

class Scene
{
public:
    Scene();
    ~Scene();

    Entity CreateEntity(const std::string& name = std::string());

    Entity CreateEntityWithUUID(const UUID& uuid, const std::string& name = std::string());

    void DestroyEntity(Entity entity);

    entt::registry& GetRegistry() noexcept { return m_Registry; }
    const entt::registry& GetRegistry() const noexcept { return m_Registry; }

    template<typename T>
    void OnComponentAdded(Entity entity, T& component)
    {
        // default - no-op
        (void)entity;
        (void)component;
    }

private:
    entt::registry m_Registry;

    friend class Entity;
};
