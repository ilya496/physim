#pragma once

#include "Asset.h"
#include "AssetMetadata.h"
#include <unordered_map>
#include <memory>

using AssetMap = std::unordered_map<AssetHandle, std::shared_ptr<Asset>>;
using AssetRegistry = std::unordered_map<AssetHandle, AssetMetadata>;

class AssetManager
{
public:
    virtual std::shared_ptr<Asset> GetAsset(AssetHandle handle);

    virtual bool IsAssetHandleValid(AssetHandle handle) const;
    virtual bool IsAssetLoaded(AssetHandle handle) const;
    virtual AssetType GetAssetType(AssetHandle handle) const;

    void ImportAsset(const std::filesystem::path& filePath);

    const AssetMetadata& GetMetadata(AssetHandle handle) const;
    const std::filesystem::path& GetFilePath(AssetHandle handle) const;

    const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

    void SerializeAssetRegistry();
    bool DeserializeAssetRegistry();
private:
    AssetMap m_LoadedAssets;
    AssetRegistry m_AssetRegistry;

    std::filesystem::path m_RegistryPath = "AssetRegistry.json";
};