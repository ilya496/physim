#pragma once

#include <filesystem>
#include <optional>

#include "EditorSettings.h"
#include "glm/glm.hpp"

class LauncherPanel
{
public:
    LauncherPanel(EditorSettings& settings);

    std::optional<std::filesystem::path> Draw(const glm::ivec2& framebufferSize);

private:
    void DrawHeader();
    void DrawRecentProjects();
    void DrawFooter();

    void SelectProject(int index, const std::filesystem::path& path);
    void ClearSelection();
    void HandleKeyboardNavigation();

private:
    EditorSettings& m_Settings;

    std::filesystem::path m_SelectedProject;
    int m_SelectedProjectIndex = -1;

    std::optional<std::filesystem::path> m_OpenRequested;
    bool m_RequestOpenFile = false;
    bool m_RequestNewProject = false;
};
