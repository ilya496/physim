#pragma once

#include "UUID.h"

using AssetHandle = UUID;

enum class AssetType : uint16_t
{
    None = 0,
    Scene,
    Texture,
};

inline std::string_view AssetTypeToString(AssetType type)
{
    switch (type)
    {
    case AssetType::None: return "AssetType::None";
    case AssetType::Scene: return "AssetType::Scene";
    case AssetType::Texture: return "AssetType::Texture";
    }

    return "AssetType::<Invalid>";
}

inline AssetType AssetTypeFromString(std::string_view assetType)
{
    if (assetType == "AssetType::None") return AssetType::None;
    if (assetType == "AssetType::Scene") return AssetType::Scene;
    if (assetType == "AssetType::Texture") return AssetType::Texture;

    return AssetType::None;
}

class Asset
{
public:
    AssetHandle Handle;

    virtual AssetType GetType() const = 0;
};