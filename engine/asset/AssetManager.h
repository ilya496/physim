#pragma once

#include "Asset.h"
#include "AssetMetadata.h"
#include "project/Project.h"
#include <unordered_map>
#include <memory>
#include "render/MeshPrimitive.h"

using AssetMap = std::unordered_map<AssetHandle, std::shared_ptr<Asset>>;
using AssetRegistry = std::unordered_map<AssetHandle, AssetMetadata>;

struct MaterialDesc
{
    glm::vec3 DiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 SpecularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float Shininess = 32.0f;

    AssetHandle DiffuseMap;
};

class AssetManager
{
public:
    template<typename T>
    static std::shared_ptr<T> GetAsset(AssetHandle handle)
    {
        std::shared_ptr<Asset> asset = Project::GetActive()->GetAssetManager()->GetAsset(handle);
        return static_pointer_cast<T>(asset);
    }

    std::shared_ptr<Asset> GetAsset(AssetHandle handle);

    bool IsAssetHandleValid(AssetHandle handle) const;
    bool IsAssetLoaded(AssetHandle handle) const;
    AssetType GetAssetType(AssetHandle handle) const;

    AssetHandle ImportAsset(const std::filesystem::path& filePath);
    AssetHandle CreateMaterial(const MaterialDesc& desc);
    AssetHandle GetDefaultMaterial();
    AssetHandle CreatePrimitiveMesh(MeshPrimitive primitive);
    AssetHandle GetDefaultMesh(MeshPrimitive primitive);

    const AssetMetadata& GetMetadata(AssetHandle handle) const;
    const std::filesystem::path& GetFilePath(AssetHandle handle) const;

    const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

    void SerializeMaterial(const MaterialDesc& desc, const std::filesystem::path& path);
    void SerializeAssetRegistry();
    bool DeserializeAssetRegistry();
private:
    AssetMap m_LoadedAssets;
    AssetMap m_RuntimeAssets; // not serialized
    std::unordered_map<MeshPrimitive, AssetHandle> m_DefaultMeshes;
    AssetRegistry m_AssetRegistry;
};