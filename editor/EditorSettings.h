#pragma once

#include <filesystem>
#include <vector>

struct EditorSettings
{
    std::vector<std::filesystem::path> RecentProjects;
    std::filesystem::path LastProject;

    static EditorSettings Load();
    void Save() const;

    void AddRecentProject(const std::filesystem::path& path);
};
