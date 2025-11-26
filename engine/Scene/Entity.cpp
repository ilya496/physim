#include "Entity.h"

Entity::Entity(entt::entity handle, Scene* scene)
    : m_Handle(handle), m_Scene(scene) {
}