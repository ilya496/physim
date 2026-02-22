#pragma once

#include <cstdint>

enum class MeshPrimitive : uint16_t
{
    CUBE = 0,
    PLANE,
    UV_SPHERE,
    ICO_SPHERE,
    CYLINDER
};