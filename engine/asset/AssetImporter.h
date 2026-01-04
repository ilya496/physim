#pragma once

#include <memory>

#include "Asset.h"
#include "AssetMetadata.h"
#include "render/Model.h"

#include <assimp/scene.h>

class AssetImporter
{
public:
    static std::shared_ptr<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);

private:
    static std::shared_ptr<TextureAsset> ImportTexture(AssetHandle handle, const AssetMetadata& metadata);
    static std::shared_ptr<MeshAsset> ImportMesh(AssetHandle handle, const AssetMetadata& metadata);
    static std::shared_ptr<MaterialAsset> ImportMaterial(AssetHandle handle, const AssetMetadata& metadata);

    static void ProcessAssimpMesh(
        aiMesh* mesh,
        const aiScene* scene,
        std::vector<Vertex>& outVertices,
        std::vector<uint32_t>& outIndices
    );

    static void ProcessAssimpNode(
        aiNode* node,
        const aiScene* scene,
        std::vector<Vertex>& vertices,
        std::vector<uint32_t>& indices
    );
    // static std::shared_ptr<ShaderAsset> ImportShader(AssetHandle handle, const AssetMetadata& metadata);
    // static std::shared_ptr<MaterialAsset> CreateDefaultMaterial();
};