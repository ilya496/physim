#pragma once

#include "UUID.h"
#include "Scene.h"
#include "Components.h"

#include "entt.hpp"

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        T& component = m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Args>(args)...);
        m_Scene->OnComponentAdded<T>(*this, component);
        return component;
    }

    template<typename T, typename... Args>
    T& AddOrReplaceComponent(Args&&... args)
    {
        T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_Handle, std::forward<Args>(args)...);
        m_Scene->OnComponentAdded<T>(*this, component);
        return component;
    }

    template<typename T>
    T& GetComponent()
    {
        return m_Scene->m_Registry.get<T>(m_Handle);
    }

    template<typename T>
    bool HasComponent()
    {
        return m_Scene->m_Registry.has<T>(m_Handle);
    }

    template<typename T>
    void RemoveComponent()
    {
        m_Scene->m_Registry.remove<T>(m_Handle);
    }

    operator bool() const { return m_Handle != entt::null; }
    operator entt::entity() const { return m_Handle; }
    operator uint32_t() const { return (uint32_t)m_Handle; }

    UUID GetUUID() { return GetComponent<IDComponent>().ID; }
    const std::string& GetName() { return GetComponent<TagComponent>().Tag; }

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