#pragma once

#include "AssetManagerBase.h"
#include "AssetMetadata.h"
#include <unordered_map>

class EditorAssetManager : public AssetManagerBase
{
public:
    virtual std::shared_ptr<Asset> GetAsset(AssetHandle handle) const override;

    virtual bool IsAssetHandleValid(AssetHandle handle) const override;
    virtual bool IsAssetLoaded(AssetHandle handle) const override;

    const AssetMetadata& GetMetadata(AssetHandle handle) const;
private:
    std::unordered_map<AssetHandle, std::shared_ptr<Asset>> m_LoadedAssets;
    std::unordered_map<AssetHandle, AssetMetadata> m_AssetRegistry;
};