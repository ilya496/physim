#pragma once

#include <memory>

#include "Asset.h"

class AssetManagerBase
{
public:
    virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
    virtual bool IsAssetLoaded(AssetHandle handle) const = 0;
    virtual std::shared_ptr<Asset> GetAsset(AssetHandle handle) const = 0;
};