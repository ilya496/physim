#include "AssetManager.h"
#include "AssetImporter.h"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>

// How AssetMetadata is serialized
static void to_json(json& j, const AssetMetadata& meta)
{
    j = json{
        {"Type",    (int)meta.Type},
        {"FilePath", meta.FilePath.string()}
    };
}

// How AssetMetadata is de-serialized
static void from_json(const json& j, AssetMetadata& meta)
{
    meta.Type = (AssetType)j.at("Type").get<int>();
    meta.FilePath = j.at("FilePath").get<std::string>();
}

static std::unordered_map<std::filesystem::path, AssetType> s_AssetExtensionMap = {
    { ".scene", AssetType::Scene },
    { ".png", AssetType::Texture },
    { ".jpg", AssetType::Texture },
    { ".jpeg", AssetType::Texture }
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
    // 1. check if handle is valid
    if (!IsAssetHandleValid(handle))
        return nullptr;

    // 2. check if asset needs load (and if so, load) 
    std::shared_ptr<Asset> asset;
    if (IsAssetLoaded(handle))
    {
        asset = m_LoadedAssets.at(handle);
    }
    else
    {
        // load asset
        const AssetMetadata& metadata = GetMetadata(handle);
        asset = AssetImporter::ImportAsset(handle, metadata);
        if (!asset) {} // import failed
    }

    // 3. return asset
    return asset;
}

void AssetManager::ImportAsset(const std::filesystem::path& filePath)
{
    AssetHandle handle;
    AssetMetadata metadata;
    metadata.FilePath = filePath;
    metadata.Type = GetAssetTypeFromFileExtension(filePath.extension());

    if (metadata.Type == AssetType::None)
        return;

    std::cout << "asset started to create\n";
    std::shared_ptr<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
    if (asset)
    {
        std::cout << "asset created\n";
        asset->Handle = handle;
        m_LoadedAssets[handle] = asset;
        m_AssetRegistry[handle] = metadata;
        SerializeAssetRegistry();
        std::cout << "registry serialized\n";
    }
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

void AssetManager::SerializeAssetRegistry()
{
    json root;

    for (const auto& [handle, metadata] : m_AssetRegistry)
    {
        // store by UUID string key
        root[handle] = metadata;
    }

    std::ofstream stream(m_RegistryPath);
    if (!stream.is_open())
        return; // optionally log

    stream << root.dump(4);   // pretty print
}

bool AssetManager::DeserializeAssetRegistry()
{
    if (!std::filesystem::exists(m_RegistryPath))
        return false;

    std::ifstream stream(m_RegistryPath);
    if (!stream.is_open())
        return false;

    json root;
    stream >> root;

    m_AssetRegistry.clear();

    for (auto& [key, value] : root.items())
    {
        AssetHandle handle(key);      // construct UUID from string
        AssetMetadata meta = value.get<AssetMetadata>();
        m_AssetRegistry.emplace(handle, meta);
    }

    return true;
}