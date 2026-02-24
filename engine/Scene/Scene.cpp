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
    // m_Registry.emplace<TransformComponent>(handle); // default transform

    // Notify hooks if needed (optional; we already called emplace directly)
    OnComponentAdded<IDComponent>(entity, m_Registry.get<IDComponent>(handle));
    OnComponentAdded<TagComponent>(entity, m_Registry.get<TagComponent>(handle));
    // OnComponentAdded<TransformComponent>(entity, m_Registry.get<TransformComponent>(handle));

    return entity;
}

Entity Scene::CreateEntityWithUUID(const UUID& uuid, const std::string& name)
{
    entt::entity handle = m_Registry.create();
    Entity entity(handle, this);

    m_Registry.emplace<IDComponent>(handle, uuid);
    m_Registry.emplace<TagComponent>(handle, name.empty() ? std::string("Entity") : name);
    // m_Registry.emplace<TransformComponent>(handle);

    OnComponentAdded<IDComponent>(entity, m_Registry.get<IDComponent>(handle));
    OnComponentAdded<TagComponent>(entity, m_Registry.get<TagComponent>(handle));
    // OnComponentAdded<TransformComponent>(entity, m_Registry.get<TransformComponent>(handle));

    return entity;
}

Entity Scene::CreateMeshEntity(const std::string& name, AssetHandle meshHandle, AssetHandle materialHandle)
{
    Entity entity = CreateEntity(name);

    auto& transform = entity.AddComponent<TransformComponent>();
    auto& meshRenderer = entity.AddComponent<MeshRenderComponent>();
    auto& rigidBody = entity.AddComponent<RigidBodyComponent>();
    auto& boxCollider = entity.AddComponent<BoxColliderComponent>();

    meshRenderer.Mesh = meshHandle;
    meshRenderer.Material = materialHandle;

    return entity;
}

Entity Scene::CreateLightEntity(const std::string& name)
{
    Entity entity = CreateEntity(name);

    auto& transform = entity.AddComponent<TransformComponent>();
    auto& light = entity.AddComponent<LightComponent>();

    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    if (!entity) return;
    m_Registry.destroy(static_cast<entt::entity>(entity));
}

namespace
{
    template<typename... Component>
    void CopyComponents(
        entt::registry& dst,
        const entt::registry& src,
        const std::unordered_map<UUID, entt::entity>& entityMap)
    {
        ([&]()
            {
                auto view = src.view<Component>();
                for (auto srcEntity : view)
                {
                    const auto& srcComponent = src.get<Component>(srcEntity);
                    const auto& id = src.get<IDComponent>(srcEntity).ID;

                    entt::entity dstEntity = entityMap.at(id);
                    dst.emplace_or_replace<Component>(dstEntity, srcComponent);
                }
            }(), ...);
    }
}

std::shared_ptr<Scene> Scene::Copy() const
{
    auto newScene = std::make_shared<Scene>();

    std::unordered_map<UUID, entt::entity> entityMap;

    // 1. Create entities
    auto idView = m_Registry.view<IDComponent, TagComponent>();
    for (auto e : idView)
    {
        const auto& id = idView.get<IDComponent>(e);
        const auto& tag = idView.get<TagComponent>(e);

        Entity newEntity = newScene->CreateEntityWithUUID(id.ID, tag.Tag);
        entityMap[id.ID] = (entt::entity)newEntity;
    }

    // 2. Copy components
    CopyComponents<
        TransformComponent,
        MeshRenderComponent,
        RigidBodyComponent,
        BoxColliderComponent,
        SphereColliderComponent,
        LightComponent,
        DistanceJointComponent
    >(newScene->m_Registry, m_Registry, entityMap);

    return newScene;
}

