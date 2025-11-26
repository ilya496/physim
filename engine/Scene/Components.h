#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "UUID.h"

struct IDComponent
{
    UUID ID;

    IDComponent() = default;
    IDComponent(const UUID& id) : ID(id) {}
};

struct TagComponent
{
    std::string Tag;

    TagComponent() = default;
    TagComponent(const std::string& tag) : Tag(tag) {}
};

struct TransformComponent
{
    glm::vec3 Translation{ 0.0f, 0.0f, 0.0f };
    glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f, 1.0f, 1.0f };

    TransformComponent() = default;
    TransformComponent(const glm::vec3& t) : Translation(t) {}
    TransformComponent(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
        : Translation(t), Rotation(r), Scale(s) {
    }

    glm::mat4 GetTransform() const
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), Translation);
        transform *= glm::mat4_cast(Rotation);
        transform = glm::scale(transform, Scale);
        return transform;
    }
};