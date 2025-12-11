#include "AssetManager.h"
#include "AssetImporter.h"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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

    std::shared_ptr<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
    if (asset)
    {
        asset->Handle = handle;
        m_LoadedAssets[handle] = asset;
        m_AssetRegistry[handle] = metadata;
        SerializeAssetRegistry();
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
    // auto path = Project::GetActiveAssetRegistryPath();
    auto path = "AssetRegistry.json";

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

    std::ofstream fout(path);
    if (fout.is_open())
        fout << root.dump(4);
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
