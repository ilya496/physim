#pragma once

#include <string>
#include <entt/entity/registry.hpp>

#include "core/UUID.h"
#include "Components.h"
#include "Scene.h"

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        auto& registry = m_Scene->GetRegistry();
        T& component = registry.emplace<T>(m_Handle, std::forward<Args>(args)...);
        m_Scene->OnComponentAdded<T>(*this, component);
        return component;
    }

    template<typename T, typename... Args>
    T& AddOrReplaceComponent(Args&&... args)
    {
        auto& registry = m_Scene->GetRegistry();
        T& component = registry.emplace_or_replace<T>(m_Handle, std::forward<Args>(args)...);
        m_Scene->OnComponentAdded<T>(*this, component);
        return component;
    }

    template<typename T>
    T& GetComponent()
    {
        return m_Scene->GetRegistry().get<T>(m_Handle);
    }

    template<typename T>
    const T& GetComponent() const
    {
        return m_Scene->m_Registry.get<T>(m_Handle);
    }


    template<typename T>
    bool HasComponent() const
    {
        return m_Scene->GetRegistry().all_of<T>(m_Handle);
    }

    template<typename T>
    void RemoveComponent()
    {
        m_Scene->GetRegistry().remove<T>(m_Handle);
    }

    explicit operator bool() const { return m_Handle != entt::null; }
    operator entt::entity() const { return m_Handle; }
    uint32_t GetHandle() const { return static_cast<uint32_t>(m_Handle); }

    UUID GetUUID() const
    {
        if (HasComponent<IDComponent>())
            return GetComponent<IDComponent>().ID;
        return UUID{};
    }

    const std::string& GetName() const
    {
        static std::string empty = "";
        if (HasComponent<TagComponent>())
            return GetComponent<TagComponent>().Tag;
        return empty;
    }

    bool operator==(const Entity& other) const
    {
        return m_Handle == other.m_Handle && m_Scene == other.m_Scene;
    }

    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }
private:
    entt::entity m_Handle{ entt::null };
    Scene* m_Scene = nullptr;
};