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

    // Create an entity with auto-generated UUID and optional name
    Entity CreateEntity(const std::string& name = std::string());

    // Create an entity with specified UUID (for deserialization)
    Entity CreateEntityWithUUID(const UUID& uuid, const std::string& name = std::string());

    // Destroy entity
    void DestroyEntity(Entity entity);

    // Access registry (needed by Entity template methods)
    entt::registry& GetRegistry() noexcept { return m_Registry; }
    const entt::registry& GetRegistry() const noexcept { return m_Registry; }

    // Hook called after adding a component. Default does nothing,
    // but you can specialize or overload it for particular types.
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
