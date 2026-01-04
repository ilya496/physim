#pragma once

#include <string>
#include <entt/entity/registry.hpp>

#include "core/UUID.h"
#include "Components.h"

class Entity;

class Scene
{
public:
    Scene();
    ~Scene();

    std::shared_ptr<Scene> Copy() const;

    Entity CreateEntity(const std::string& name = std::string());
    Entity CreateEntityWithUUID(const UUID& uuid, const std::string& name = std::string());
    Entity CreateMeshEntity(const std::string& name, AssetHandle meshHandle, AssetHandle materialHandle);
    Entity CreateLightEntity(const std::string& name);

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
