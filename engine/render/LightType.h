#pragma once

#include <cstdint>

enum class LightType : uint16_t
{
    None = 0,
    Directional,
    Point,
    Spot
};