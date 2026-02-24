#pragma once

#include <glm/glm.hpp>

// struct AABB
// {
//     glm::vec3 Min;
//     glm::vec3 Max;

//     bool Overlaps(const AABB& other) const
//     {
//         return (Min.x <= other.Max.x && Max.x >= other.Min.x) &&
//             (Min.y <= other.Max.y && Max.y >= other.Min.y) &&
//             (Min.z <= other.Max.z && Max.z >= other.Min.z);
//     }
// };

struct AABB {
    glm::vec3 Min{ FLT_MAX,  FLT_MAX,  FLT_MAX };
    glm::vec3 Max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    AABB() = default;
    AABB(const glm::vec3& mn, const glm::vec3& mx) : Min(mn), Max(mx) {}

    bool Overlaps(const AABB& o) const {
        return Max.x >= o.Min.x && Min.x <= o.Max.x &&
            Max.y >= o.Min.y && Min.y <= o.Max.y &&
            Max.z >= o.Min.z && Min.z <= o.Max.z;
    }
    glm::vec3 Center()  const { return (Min + Max) * 0.5f; }
    glm::vec3 Extents() const { return (Max - Min) * 0.5f; }
};