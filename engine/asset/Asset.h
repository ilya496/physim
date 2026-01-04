#pragma once

#include "core/UUID.h"

using AssetHandle = UUID;

enum class AssetType : uint16_t
{
    None = 0,
    Texture,
    Mesh,
    Material
};

inline std::string_view AssetTypeToString(AssetType type)
{
    switch (type)
    {
    case AssetType::None: return "AssetType::None";
    case AssetType::Mesh: return "AssetType::Mesh";
    case AssetType::Texture: return "AssetType::Texture";
    case AssetType::Material: return "AssetType::Material";
    }

    return "AssetType::<Invalid>";
}

inline AssetType AssetTypeFromString(std::string_view assetType)
{
    if (assetType == "AssetType::None") return AssetType::None;
    if (assetType == "AssetType::Mesh") return AssetType::Mesh;
    if (assetType == "AssetType::Texture") return AssetType::Texture;
    if (assetType == "AssetType::Material") return AssetType::Material;

    return AssetType::None;
}

class Asset
{
public:
    AssetHandle Handle;

    virtual AssetType GetType() const = 0;
};