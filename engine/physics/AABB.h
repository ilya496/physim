#pragma once

#include <glm/glm.hpp>

struct AABB
{
    glm::vec3 Min;
    glm::vec3 Max;

    bool Overlaps(const AABB& other) const
    {
        return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
            (Min.y <= other.Max.y && Max.y >= other.Min.y) &&
            (Min.z <= other.Max.z && Max.z >= other.Min.z);
    }
};