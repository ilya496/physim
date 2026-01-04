#pragma once

#include <entt/entity/registry.hpp>

class EditorContext
{
public:
    static void SetSelectedEntity(entt::entity entity) { s_SelectedEntity = entity; }
    static entt::entity GetSelectedEntity() { return s_SelectedEntity; }

private:
    inline static entt::entity s_SelectedEntity = entt::null;
};
