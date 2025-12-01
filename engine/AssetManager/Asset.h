#pragma once

#include "UUID.h"

using AssetHandle = UUID;

enum class AssetType
{
    None = 0,
    Scene,
    Texture2D,
};

class Asset
{
public:
    AssetHandle Handle;

    virtual AssetType GetType() const = 0;
};