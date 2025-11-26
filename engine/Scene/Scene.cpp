#include "Scene.h"
#include "Entity.h"

// Scene lifecycle
Scene::Scene() = default;
Scene::~Scene() = default;

Entity Scene::CreateEntity(const std::string& name)
{
    entt::entity handle = m_Registry.create();
    Entity entity(handle, this);

    // Add core components
    m_Registry.emplace<IDComponent>(handle, UUID());
    m_Registry.emplace<TagComponent>(handle, name.empty() ? std::string("Entity") : name);
    m_Registry.emplace<TransformComponent>(handle); // default transform

    // Notify hooks if needed (optional; we already called emplace directly)
    OnComponentAdded<IDComponent>(entity, m_Registry.get<IDComponent>(handle));
    OnComponentAdded<TagComponent>(entity, m_Registry.get<TagComponent>(handle));
    OnComponentAdded<TransformComponent>(entity, m_Registry.get<TransformComponent>(handle));

    return entity;
}

Entity Scene::CreateEntityWithUUID(const UUID& uuid, const std::string& name)
{
    entt::entity handle = m_Registry.create();
    Entity entity(handle, this);

    m_Registry.emplace<IDComponent>(handle, uuid);
    m_Registry.emplace<TagComponent>(handle, name.empty() ? std::string("Entity") : name);
    m_Registry.emplace<TransformComponent>(handle);

    OnComponentAdded<IDComponent>(entity, m_Registry.get<IDComponent>(handle));
    OnComponentAdded<TagComponent>(entity, m_Registry.get<TagComponent>(handle));
    OnComponentAdded<TransformComponent>(entity, m_Registry.get<TransformComponent>(handle));

    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    if (!entity) return;
    m_Registry.destroy(static_cast<entt::entity>(entity));
}
