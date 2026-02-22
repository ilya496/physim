#include "AssetManager.h"
#include "AssetImporter.h"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <iostream>

static std::unordered_map<std::filesystem::path, AssetType> s_AssetExtensionMap = {
    { ".png", AssetType::Texture },
    { ".jpg", AssetType::Texture },
    { ".jpeg", AssetType::Texture },
    { ".obj", AssetType::Mesh },
    { ".stl", AssetType::Mesh },
    { ".gltf", AssetType::Mesh },
    { ".glb", AssetType::Mesh },
    { ".mat", AssetType::Material }
};

static AssetType GetAssetTypeFromFileExtension(const std::filesystem::path& extension)
{
    if (s_AssetExtensionMap.find(extension) == s_AssetExtensionMap.end())
    {
        return AssetType::None;
    }

    return s_AssetExtensionMap.at(extension);
}

bool AssetManager::IsAssetHandleValid(AssetHandle handle) const
{
    if (handle == 0)
        return false;

    return m_AssetRegistry.find(handle) != m_AssetRegistry.end();
}

bool AssetManager::IsAssetLoaded(AssetHandle handle) const
{
    return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
}

std::shared_ptr<Asset> AssetManager::GetAsset(AssetHandle handle)
{
    if (m_RuntimeAssets.find(handle) != m_RuntimeAssets.end())
        return m_RuntimeAssets[handle];

    if (!IsAssetHandleValid(handle))
        return nullptr;

    std::shared_ptr<Asset> asset;
    if (IsAssetLoaded(handle))
    {
        asset = m_LoadedAssets.at(handle);
    }
    else
    {
        const AssetMetadata& metadata = GetMetadata(handle);
        asset = AssetImporter::ImportAsset(handle, metadata);
        if (!asset) {} // import failed
        m_LoadedAssets[handle] = asset;
    }

    return asset;
}

AssetHandle AssetManager::ImportAsset(const std::filesystem::path& filePath)
{
    AssetHandle handle;
    AssetMetadata metadata;
    metadata.FilePath = filePath;
    metadata.Type = GetAssetTypeFromFileExtension(filePath.extension());

    if (metadata.Type == AssetType::None)
        return handle;

    std::shared_ptr<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
    if (asset)
    {
        asset->Handle = handle;
        m_LoadedAssets[handle] = asset;
        m_AssetRegistry[handle] = metadata;
        SerializeAssetRegistry();
    }

    return handle;
}

AssetHandle AssetManager::CreateMaterial(const MaterialDesc& desc)
{
    AssetHandle handle;

    std::filesystem::path path = "materials/" + handle.string() + ".mat";

    SerializeMaterial(desc, path);

    AssetMetadata metadata;
    metadata.Type = AssetType::Material;
    metadata.FilePath = path;

    m_AssetRegistry.emplace(handle, metadata);
    SerializeAssetRegistry();

    return handle;
}

AssetHandle AssetManager::GetDefaultMaterial()
{
    static AssetHandle s_DefaultMaterial = 0;

    if (s_DefaultMaterial != 0)
        return s_DefaultMaterial;

    MaterialDesc desc;
    desc.DiffuseColor = { 0.21f, 0.27f, 0.31f };
    desc.SpecularColor = { 0.21f, 0.27f, 0.31f };
    desc.Shininess = 32.0f;
    desc.DiffuseMap = 0;

    s_DefaultMaterial = CreateMaterial(desc);
    return s_DefaultMaterial;
}

// AssetHandle AssetManager::CreateCube()
// {
//     AssetHandle handle;
//     std::shared_ptr<Mesh> mesh = Mesh::Generate(MeshPrimitive::CUBE);

//     auto asset = std::make_shared<MeshAsset>();
//     asset->Handle = handle;
//     asset->MeshData = mesh;

//     m_RuntimeAssets.emplace(handle, asset);

//     return handle;
// }

// AssetHandle AssetManager::GetDefaultCube()
// {
//     static AssetHandle s_DefaultCube = 0;

//     if (s_DefaultCube != 0)
//         return s_DefaultCube;

//     s_DefaultCube = CreateCube();
//     return s_DefaultCube;
// }

AssetHandle AssetManager::CreatePrimitiveMesh(MeshPrimitive primitive)
{
    AssetHandle handle;

    auto mesh = Mesh::Generate(primitive);

    auto asset = std::make_shared<MeshAsset>();
    asset->Handle = handle;
    asset->MeshData = mesh;

    m_RuntimeAssets.emplace(handle, asset);

    return handle;
}

AssetHandle AssetManager::GetDefaultMesh(MeshPrimitive primitive)
{
    auto it = m_DefaultMeshes.find(primitive);
    if (it != m_DefaultMeshes.end())
        return it->second;

    AssetHandle handle = CreatePrimitiveMesh(primitive);
    m_DefaultMeshes.emplace(primitive, handle);

    return handle;
}

AssetType AssetManager::GetAssetType(AssetHandle handle) const
{
    if (!IsAssetHandleValid(handle))
        return AssetType::None;

    return m_AssetRegistry.at(handle).Type;
}

const AssetMetadata& AssetManager::GetMetadata(AssetHandle handle) const
{
    static AssetMetadata s_NullMetadata;
    auto it = m_AssetRegistry.find(handle);
    if (it == m_AssetRegistry.end())
        return s_NullMetadata;

    return it->second;
}

const std::filesystem::path& AssetManager::GetFilePath(AssetHandle handle) const
{
    return GetMetadata(handle).FilePath;
}

void AssetManager::SerializeMaterial(const MaterialDesc& desc, const std::filesystem::path& path)
{
    json j;
    j["DiffuseColor"] = { desc.DiffuseColor.r, desc.DiffuseColor.g, desc.DiffuseColor.b };
    j["SpecularColor"] = { desc.SpecularColor.r, desc.SpecularColor.g, desc.SpecularColor.b };
    j["Shininess"] = desc.Shininess;
    j["DiffuseMap"] = desc.DiffuseMap.string();

    std::ofstream out(Project::GetActiveAssetFileSystemPath(path));
    out << j.dump(4);
}

void AssetManager::SerializeAssetRegistry()
{
    std::filesystem::path registryPath = Project::GetActiveAssetRegistryPath();

    json root;
    root["AssetRegistry"] = json::array();

    for (const auto& [handle, metadata] : m_AssetRegistry)
    {
        json entry;
        entry["Handle"] = handle.string();
        entry["FilePath"] = metadata.FilePath.generic_string();
        entry["Type"] = AssetTypeToString(metadata.Type);

        root["AssetRegistry"].push_back(std::move(entry));
    }

    std::ofstream fout(registryPath);
    if (fout.is_open())
        fout << root.dump(4);
}

bool AssetManager::DeserializeAssetRegistry()
{
    std::filesystem::path registryPath = Project::GetActiveAssetRegistryPath();

    if (!std::filesystem::exists(registryPath))
        return false;

    std::ifstream stream(registryPath);
    if (!stream.is_open())
        return false;

    json root;
    stream >> root;

    if (!root.contains("AssetRegistry"))
        return false;

    m_AssetRegistry.clear();

    for (const auto& entry : root["AssetRegistry"])
    {
        std::string handleStr = entry.at("Handle").get<std::string>();
        std::string filepath = entry.at("FilePath").get<std::string>();
        std::string typeStr = entry.at("Type").get<std::string>();

        AssetHandle handle(handleStr);

        AssetMetadata meta;
        meta.FilePath = filepath;
        meta.Type = AssetTypeFromString(typeStr);

        m_AssetRegistry.emplace(handle, meta);
    }

    return true;
}
