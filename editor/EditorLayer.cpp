#include "EditorLayer.h"

#include "core/EventBus.h"
#include "project/Project.h"
#include "utils/FileDialog.h"
#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

EditorLayer::EditorLayer(Window* window)
    : m_Window(window)
{
}

EditorLayer::~EditorLayer()
{
}

void EditorLayer::OnAttach()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_Window->GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    SetupImGuiFonts("../JetBrainsMono-Regular.ttf");

    m_Settings = EditorSettings::Load();

    m_NewFrameSub = EventBus::Subscribe<NewFrameRenderedEvent>(
        [this](const NewFrameRenderedEvent& e)
        {
            m_ViewportTexture = e.ColorAttachment;
            m_ViewportWidth = e.Width;
            m_ViewportHeight = e.Height;
        }
    );
}

void EditorLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::OnUpdate(float dt)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    BeginDockspace();
}

void EditorLayer::OnRender()
{
    if (m_State == EditorState::Launcher)
    {
        DrawLauncher();
    }
    else
    {
        DrawViewport();
        DrawAssetsPanel();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::SetupImGuiFonts(const char* path)
{
    ImGuiIO& io = ImGui::GetIO();

    float xs, ys;
    glfwGetWindowContentScale(m_Window->GetNativeWindow(), &xs, &ys);

    float fontSize = 16.0f * xs;

    io.Fonts->Clear();
    io.FontDefault = io.Fonts->AddFontFromFileTTF(path, fontSize);
    io.Fonts->Build();
}

void EditorLayer::BeginDockspace()
{
    static bool dockspaceOpen = true;
    static bool optFullscreen = true;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    if (optFullscreen)
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Dockspace Window", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar();

    ImGuiID dockspaceID = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (auto project = Project::GetActive())
            {
                ImGui::TextDisabled("Project: %s", project->GetConfig().Name.c_str());
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Save Project"))
            {
                // Project::SaveActive(...);
            }

            if (ImGui::MenuItem("Open Project..."))
            {
                auto path = FileDialog::OpenFile("Physim Project", "physim");
                if (!path.empty())
                    OpenProject(path);
            }

            bool hasProject = Project::GetActive() != nullptr;
            if (ImGui::MenuItem("Close Project", nullptr, false, hasProject))
            {
                Project::GetActive().reset();
                m_SelectedProject.clear();
                m_State = EditorState::Launcher;
            }

            if (ImGui::MenuItem("Exit"))
                EventBus::Publish(WindowCloseEvent{});

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorLayer::DrawLauncher()
{
    ImGui::Begin("Physim Launcher",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    const ImVec2 size(800, 450);
    auto fb = m_Window->GetFramebufferSize();

    ImGui::SetWindowSize(size, ImGuiCond_Always);
    ImGui::SetWindowPos(
        ImVec2((fb.x - size.x) * 0.5f,
            (fb.y - size.y) * 0.5f));

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 520);

    ImGui::Text("Recent Projects");
    ImGui::Spacing();

    ImGui::BeginChild("RecentProjects", ImVec2(0, 300), true);

    for (const auto& project : m_Settings.RecentProjects)
    {
        bool selected = (project == m_SelectedProject);

        // Display name + full path to avoid ambiguity
        std::string label =
            project.filename().string() + "##" + project.string();

        if (ImGui::Selectable(label.c_str(), selected,
            ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(0, 28)))
        {
            m_SelectedProject = project;

            if (ImGui::IsMouseDoubleClicked(0))
                OpenProject(project);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%s", project.parent_path().string().c_str());
    }

    if (ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        m_SelectedProject.clear();
    }

    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::Text("Actions");
    ImGui::Spacing();

    if (ImGui::Button("Open Selected", ImVec2(-1, 0)))
    {
        if (!m_SelectedProject.empty())
            OpenProject(m_SelectedProject);
    }

    if (ImGui::Button("Open Project...", ImVec2(-1, 0)))
    {
        auto path = FileDialog::OpenFile("Physim Project", "physim");
        if (!path.empty())
            OpenProject(path);
    }

    if (ImGui::Button("New Project...", ImVec2(-1, 0)))
    {
        CreateNewProject();
    }

    ImGui::Columns(1);
    ImGui::Separator();

    if (!m_SelectedProject.empty())
    {
        ImGui::TextDisabled("Selected:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", m_SelectedProject.string().c_str());
    }
    else
    {
        ImGui::TextDisabled("No project selected");
    }

    ImGui::End();
}


void EditorLayer::DrawRecentProjects()
{
    ImGui::Text("Recent Projects");

    for (const auto& project : m_Settings.RecentProjects)
    {
        if (ImGui::Selectable(project.filename().string().c_str()))
        {
            OpenProject(project);
        }
    }
}

void EditorLayer::OpenProject(const std::filesystem::path& path)
{
    if (Project::Load(path))
    {
        std::string title = "Physim Editor";
        if (auto project = Project::GetActive())
            title += " - " + project->GetConfig().Name;

        m_Window->SetTitle(title.c_str());

        m_Settings.AddRecentProject(path);
        m_Settings.Save();

        m_State = EditorState::Editor;
    }
}

void EditorLayer::CreateNewProject()
{
    auto folder = FileDialog::SelectFolder("New Project");
    if (folder.empty())
        return;

    auto project = Project::New();
    Project::SaveActive(folder / "NewProject.physim");

    m_State = EditorState::Editor;
}

void EditorLayer::DrawAssetsPanel()
{
    ImGui::Begin("Asset");

    ImGui::End();
}

void EditorLayer::DrawViewport()
{
    ImGui::Begin("Viewport");

    if (m_ViewportTexture)
    {
        ImVec2 size = ImGui::GetContentRegionAvail();

        ImGui::Image(
            (ImTextureID)(uint64_t)m_ViewportTexture,
            size,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
    }

    ImGui::End();
}
