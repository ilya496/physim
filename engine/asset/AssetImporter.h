#pragma once

#include <memory>

#include "Asset.h"
#include "AssetMetadata.h"

struct Texture : Asset
{
    virtual AssetType GetType() const override { return AssetType::Texture; }
}; // TODO: implement
struct Scene : Asset {}; // TODO: implement

class AssetImporter
{
public:
    static std::shared_ptr<Asset> ImportAsset(AssetHandle hanadle, const AssetMetadata& metadata);

private:
    static std::shared_ptr<Texture> ImportTexture(AssetHandle handle, const AssetMetadata& metadata);
    static std::shared_ptr<Scene> ImportScene(AssetHandle handle, const AssetMetadata& metadata);
};