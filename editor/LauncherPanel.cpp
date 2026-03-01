#include "LauncherPanel.h"

#include "imgui.h"
#include "utils/FileDialog.h"
#include <iostream>

LauncherPanel::LauncherPanel(EditorSettings& settings)
    : m_Settings(settings)
{
}

std::optional<std::filesystem::path> LauncherPanel::Draw(const glm::ivec2& fb)
{
    std::optional<std::filesystem::path> result;

    ImGui::Begin("Physim Launcher",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    const ImVec2 windowSize(850, 500);

    ImGui::SetWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetWindowPos(
        ImVec2((fb.x - windowSize.x) * 0.5f,
            (fb.y - windowSize.y) * 0.5f));

    DrawHeader();
    ImGui::Separator();
    DrawRecentProjects();
    ImGui::Separator();
    DrawFooter();

    if (m_RequestOpenFile)
    {
        auto path = FileDialog::OpenFile("Physim Project", "physim");
        if (!path.empty())
            result = path;
        m_RequestOpenFile = false;
    }

    if (m_RequestNewProject)
    {
        auto folder = FileDialog::SelectFolder("New Project");
        if (!folder.empty())
            result = folder / "NewProject.physim";
        m_RequestNewProject = false;
    }

    if (m_OpenRequested)
    {
        result = m_OpenRequested;
        m_OpenRequested.reset();
    }

    ImGui::End();
    return result;
}

void LauncherPanel::DrawHeader()
{
    ImGui::Text("Recent Projects");

    const float buttonWidth = 185.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float totalWidth = buttonWidth * 2 + spacing;

    float rightX =
        ImGui::GetWindowContentRegionMax().x - totalWidth;

    ImGui::SameLine();
    ImGui::SetCursorPosX(rightX);

    if (ImGui::Button("Open Project", ImVec2(buttonWidth, 0)))
        m_RequestOpenFile = true;

    ImGui::SameLine();

    if (ImGui::Button("New Project", ImVec2(buttonWidth, 0)))
        m_RequestNewProject = true;
}

void LauncherPanel::DrawRecentProjects()
{
    ImGui::BeginChild("RecentProjects", ImVec2(0, 300), true);

    for (int i = 0; i < (int)m_Settings.RecentProjects.size(); ++i)
    {
        const auto& project = m_Settings.RecentProjects[i];
        bool selected = (i == m_SelectedProjectIndex);

        std::string label =
            project.filename().string() + "##" + project.string();

        if (ImGui::Selectable(
            label.c_str(),
            selected,
            ImGuiSelectableFlags_AllowDoubleClick |
            ImGuiSelectableFlags_SelectOnNav,
            ImVec2(0, 28)))
        {
            SelectProject(i, project);

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                m_OpenRequested = project;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%s", project.parent_path().string().c_str());

        if (selected)
            ImGui::SetItemDefaultFocus();
    }

    HandleKeyboardNavigation();

    if (ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        ClearSelection();
    }

    ImGui::EndChild();
}


void LauncherPanel::DrawFooter()
{
    if (!m_SelectedProject.empty())
    {
        ImGui::TextDisabled("Selected Project");

        ImGui::Text("Name:");
        ImGui::SameLine();
        ImGui::Text("%s",
            m_SelectedProject.filename().string().c_str());

        ImGui::Text("Path:");
        ImGui::SameLine();
        ImGui::TextDisabled("%s",
            m_SelectedProject.parent_path().string().c_str());
    }
    else
    {
        ImGui::TextDisabled("No project selected");
    }
}

void LauncherPanel::SelectProject(int index, const std::filesystem::path& path)
{
    m_SelectedProjectIndex = index;
    m_SelectedProject = path;
}

void LauncherPanel::ClearSelection()
{
    m_SelectedProjectIndex = -1;
    m_SelectedProject.clear();
}

void LauncherPanel::HandleKeyboardNavigation()
{
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        return;

    const int count = (int)m_Settings.RecentProjects.size();

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && m_SelectedProjectIndex > 0)
        --m_SelectedProjectIndex;

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
        m_SelectedProjectIndex + 1 < count)
        ++m_SelectedProjectIndex;

    if (ImGui::IsKeyPressed(ImGuiKey_Enter) &&
        m_SelectedProjectIndex >= 0 &&
        m_SelectedProjectIndex < count)
    {
        m_OpenRequested =
            m_Settings.RecentProjects[m_SelectedProjectIndex];
    }

    if (m_SelectedProjectIndex >= 0 &&
        m_SelectedProjectIndex < count)
    {
        m_SelectedProject =
            m_Settings.RecentProjects[m_SelectedProjectIndex];
    }
}