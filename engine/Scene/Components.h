#pragma once

#include <string>
#include <glm/glm.hpp>
#include "engine/UUID.h"


struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const IDComponent&) = default;
};

struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag)
        : Tag(tag) {
    }
};

struct TransformComponent
{
    glm::vec3 Transform;
    glm::vec3 Rotation;
    glm::vec3 Scale;

    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    // TransformComponent()

    glm::mat4 GetTransform() const
    {

    }
};