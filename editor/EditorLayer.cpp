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

    m_RecentProjects = {
        "Projects/TestProject/TestProject.physim",
        "Projects/Sandbox/Sandbox.physim"
    };

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
            if (ImGui::MenuItem("Exit"))
            {
                WindowCloseEvent e;
                EventBus::Publish(e);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorLayer::DrawLauncher()
{
    ImGui::Begin("Physim Launcher", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove);

    glm::ivec2 windowSize = m_Window->GetFramebufferSize();

    ImGui::SetWindowSize(ImVec2(600, 400), ImGuiCond_Always);
    ImGui::SetWindowPos(ImVec2(
        (windowSize.x - 600) * 0.5f,
        (windowSize.y - 400) * 0.5f));

    ImGui::Text("Welcome to Physim");
    ImGui::Separator();

    DrawRecentProjects();

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Open Project"))
    {
        // platform file dialog
        auto path = FileDialog::OpenFile("Physim Project", "physim");
        if (!path.empty())
        {
            std::cout << path.string();
            OpenProject(path);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("New Project"))
    {
        CreateNewProject();
    }

    ImGui::End();
}

void EditorLayer::DrawRecentProjects()
{
    ImGui::Text("Recent Projects");

    for (const auto& project : m_RecentProjects)
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
        m_State = EditorState::Editor;
        // Optional: add to recent list, save settings
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
