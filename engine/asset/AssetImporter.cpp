#include "AssetImporter.h"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

std::shared_ptr<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
{
    switch (metadata.Type)
    {
    case AssetType::Texture: return ImportTexture(handle, metadata);
    case AssetType::Mesh: return ImportMesh(handle, metadata);
    case AssetType::Material: return ImportMaterial(handle, metadata);
        // case AssetType::Shader: return ImportShader(handle, metadata);
    default:
        return nullptr;
    }
}

std::shared_ptr<TextureAsset> AssetImporter::ImportTexture(AssetHandle handle, const AssetMetadata& metadata)
{
    auto texture = std::make_shared<Texture>(metadata.FilePath, true);

    auto asset = std::make_shared<TextureAsset>();
    asset->Handle = handle;
    asset->TextureData = texture;

    return asset;
}

std::shared_ptr<MeshAsset> AssetImporter::ImportMesh(AssetHandle handle, const AssetMetadata& metadata)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        metadata.FilePath.string(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph |
        aiProcess_FlipUVs
    );

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        std::cerr << "Assimp error: " << importer.GetErrorString() << "\n";
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    ProcessAssimpNode(scene->mRootNode, scene, vertices, indices);

    if (vertices.empty() || indices.empty())
    {
        std::cerr << "Mesh import failed: no geometry\n";
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>(vertices, indices);

    auto asset = std::make_shared<MeshAsset>();
    asset->Handle = handle;
    asset->MeshData = mesh;

    return asset;
}

std::shared_ptr<MaterialAsset> AssetImporter::ImportMaterial(AssetHandle handle, const AssetMetadata& metadata)
{
    std::ifstream in(Project::GetActiveAssetDirectory() / metadata.FilePath);
    json j;
    in >> j;

    MaterialDesc desc;
    desc.DiffuseColor = {
        j["DiffuseColor"][0],
        j["DiffuseColor"][1],
        j["DiffuseColor"][2],
    };

    desc.SpecularColor = {
        j["SpecularColor"][0],
        j["SpecularColor"][1],
        j["SpecularColor"][2],
    };

    desc.Shininess = j["Shininess"];
    desc.DiffuseMap = AssetHandle(j["DiffuseMap"].get<std::string>());

    auto material = std::make_shared<Material>();
    material->ApplyDescription(desc);

    auto asset = std::make_shared<MaterialAsset>();
    asset->Handle = handle;
    asset->MaterialData = material;

    return asset;
}

void AssetImporter::ProcessAssimpMesh(
    aiMesh* mesh,
    const aiScene* scene,
    std::vector<Vertex>& outVertices,
    std::vector<uint32_t>& outIndices
)
{
    uint32_t baseVertex = static_cast<uint32_t>(outVertices.size());

    // vertices
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v{};
        v.Position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z,
        };

        if (mesh->HasNormals())
        {
            v.Normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z,
            };
        }

        if (mesh->mTextureCoords[0])
        {
            v.TexCoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y,
            };
        }
        else
        {
            v.TexCoords = { 0.0f, 0.0f };
        }

        outVertices.push_back(v);
    }

    // indices
    for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; ++j)
            outIndices.push_back(baseVertex + face.mIndices[j]);
    }
}

void AssetImporter::ProcessAssimpNode(
    aiNode* node,
    const aiScene* scene,
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices
)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessAssimpMesh(mesh, scene, vertices, indices);
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++)
        ProcessAssimpNode(node->mChildren[i], scene, vertices, indices);
}