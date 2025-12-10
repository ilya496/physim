#pragma once

#include <string>
#include <filesystem>

#include "engine/asset/AssetManager.h"

struct ProjectConfig
{
    std::string Name = "Untitled";
    AssetHandle StartScene;

    std::filesystem::path AssetDirectory;
    std::filesystem::path AssetRegistryPath;
};

class Project
{
public:
    const std::filesystem::path& GetProjectDirectory() { return m_ProjectDirectory; }
    std::filesystem::path GetAssetDirectory() { return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory; }
    std::filesystem::path GetAssetRegistryPath() { return GetAssetDirectory() / s_ActiveProject->m_Config.AssetRegistryPath; }
    std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path) { return GetAssetDirectory() / path; }

    std::filesystem::path GetAssetAbsolutePath(const std::filesystem::path& path);

    static const std::filesystem::path& GetActiveProjectDirectory()
    {
        return s_ActiveProject->GetProjectDirectory();
    }

    static std::filesystem::path GetActiveAssetDirectory()
    {
        return s_ActiveProject->GetAssetDirectory();
    }

    static std::filesystem::path GetActiveAssetRegistryPath()
    {
        return s_ActiveProject->GetAssetRegistryPath();
    }

    static std::filesystem::path GetActiveAssetFileSystemPath(const std::filesystem::path& path)
    {
        return s_ActiveProject->GetAssetFileSystemPath(path);
    }


    ProjectConfig& GetConfig() { return m_Config; }

    static std::shared_ptr<Project> GetActive() { return s_ActiveProject; }
    std::shared_ptr<AssetManager> GetAssetManager() { return m_AssetManager; }

    static std::shared_ptr<Project> New();
    static std::shared_ptr<Project> Load(const std::filesystem::path& path);
    static bool SaveActive(const std::filesystem::path& path);
private:
    ProjectConfig m_Config;
    std::filesystem::path m_ProjectDirectory;
    std::shared_ptr<AssetManager> m_AssetManager;

    inline static std::shared_ptr<Project> s_ActiveProject;
};