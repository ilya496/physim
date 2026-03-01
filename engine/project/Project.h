#pragma once

#include <string>
#include <filesystem>

#include "scene/Scene.h"

class AssetManager;

struct ProjectConfig
{
    std::string Name = "Untitled";
    AssetHandle StartScene = 0;

    std::filesystem::path AssetDirectory = "assets";
    std::filesystem::path AssetRegistryPath = "asset_registry.json";
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

    static std::filesystem::path GetActiveProjectName()
    {
        return s_ActiveProject->GetProjectDirectory() / s_ActiveProject->m_Config.Name;
    }

    ProjectConfig& GetConfig() { return m_Config; }

    static std::shared_ptr<Project> GetActive() { return s_ActiveProject; }
    std::shared_ptr<AssetManager> GetAssetManager() { return m_AssetManager; }
    std::shared_ptr<Scene> GetActiveScene() { return m_ActiveScene; }
    void SetActiveScene(std::shared_ptr<Scene> scene) { m_ActiveScene = scene; }

    static std::shared_ptr<Project> New(const std::filesystem::path& path);
    static std::shared_ptr<Project> Load(const std::filesystem::path& path);
    static bool SaveActive(const std::filesystem::path& path);
    static void Close();

private:
    ProjectConfig m_Config;
    std::filesystem::path m_ProjectDirectory;
    std::shared_ptr<AssetManager> m_AssetManager;
    std::shared_ptr<Scene> m_ActiveScene;

    inline static std::shared_ptr<Project> s_ActiveProject;
};