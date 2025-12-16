#pragma once

#include <filesystem>
#include <optional>

#include "EditorSettings.h"
#include "glm/glm.hpp"

class LauncherPanel
{
public:
    LauncherPanel(EditorSettings& settings);

    // Returns a project path when user wants to open one
    std::optional<std::filesystem::path> Draw(const glm::ivec2& framebufferSize);

private:
    void DrawHeader();
    void DrawRecentProjects();
    void DrawFooter();

private:
    EditorSettings& m_Settings;

    std::filesystem::path m_SelectedProject;
    int m_SelectedProjectIndex = -1;

    bool m_RequestOpenFile = false;
    bool m_RequestNewProject = false;
};
