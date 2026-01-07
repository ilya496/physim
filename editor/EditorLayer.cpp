#include "EditorLayer.h"

#include "core/EventBus.h"
#include "project/Project.h"
#include "utils/FileDialog.h"
#include <iostream>
#include "core/Input.h"
#include "EditorContext.h"

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
    m_LauncherPanel = std::make_unique<LauncherPanel>(m_Settings);
    m_InspectorPanel = std::make_unique<InspectorPanel>();
    m_AssetPanel = std::make_unique<AssetPanel>();
    m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>();

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

    if (Input::IsKeyPressed(KeyCode::LeftControl))
    {
        if (Input::IsKeyPressed(KeyCode::S))
        {
            Project::SaveActive(Project::GetActiveProjectName());
        }

        if (Input::IsKeyPressed(KeyCode::O))
        {
            auto path = FileDialog::OpenFile("Physim Project", "physim");
            if (!path.empty())
                OpenProject(path);
        }
    }

    BeginDockspace();
}

void EditorLayer::OnFixedUpdate(float dt)
{
    m_SceneController.Update(dt);
}

void EditorLayer::OnRender()
{
    if (m_State == EditorState::Launcher)
    {
        if (auto projectPath = m_LauncherPanel->Draw(m_Window->GetFramebufferSize()))
        {
            OpenProject(*projectPath);
            m_State = EditorState::Editor;
            m_SceneController.SetEditorScene(Project::GetActive()->GetActiveScene());
        }
    }
    else
    {
        DrawViewport();
        DrawToolbar();
        m_AssetPanel->Draw(Project::GetActive()->GetActiveScene());
        m_InspectorPanel->Draw(Project::GetActive()->GetActiveScene());
        m_SceneHierarchyPanel->Draw(Project::GetActive()->GetActiveScene());
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::SetupImGuiFonts(const char* path)
{
    ImGuiIO& io = ImGui::GetIO();

    float xs, ys;
    glfwGetWindowContentScale(m_Window->GetNativeWindow(), &xs, &ys);

    float dpiScale = xs;
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark();

    style.ScaleAllSizes(dpiScale);

    io.Fonts->Clear();
    io.FontDefault = io.Fonts->AddFontFromFileTTF(path, 16.0f * dpiScale);
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

            if (ImGui::MenuItem("Save Project", "Ctrl+S"))
            {
                Project::SaveActive(Project::GetActiveProjectName());
            }

            if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
            {
                auto path = FileDialog::OpenFile("Physim Project", "physim");
                if (!path.empty())
                    OpenProject(path);
            }

            bool hasProject = Project::GetActive() != nullptr;
            if (ImGui::MenuItem("Close Project", "Ctrl+W", false, hasProject))
            {
                Project::Close();
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

void EditorLayer::DrawToolbar()
{
    ImGui::Begin("Toolbar", nullptr);

    if (ImGui::Button("Play"))
        m_SceneController.Play();

    ImGui::SameLine();
    if (ImGui::Button("Pause"))
        m_SceneController.Pause();

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        m_SceneController.Stop();

    ImGui::End();
}


void EditorLayer::DrawViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::Begin("Viewport", nullptr);

    ImVec2 viewportMin = ImGui::GetCursorScreenPos();
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportMax = {
        viewportMin.x + viewportSize.x,
        viewportMin.y + viewportSize.y
    };

    // Draw image FIRST
    if (m_ViewportTexture)
    {
        ImGui::GetWindowDrawList()->AddImage(
            (ImTextureID)(uint64_t)m_ViewportTexture,
            viewportMin,
            viewportMax,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
    }

    ImVec2 mousePos = ImGui::GetMousePos();
    bool hovered =
        mousePos.x >= viewportMin.x &&
        mousePos.x <= viewportMax.x &&
        mousePos.y >= viewportMin.y &&
        mousePos.y <= viewportMax.y;

    if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
    {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return;
    }

    ImGui::InvisibleButton("##ViewportDropTarget", viewportSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight);

    // drag&drop
    bool dragHover = false;

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(
                "CONTENT_BROWSER_ITEM",
                ImGuiDragDropFlags_AcceptBeforeDelivery))
        {
            dragHover = true;

            if (payload->IsDelivery())
            {
                AssetHandle handle = *(AssetHandle*)payload->Data;
                auto project = Project::GetActive();
                auto assetManager = project->GetAssetManager();
                project->GetActiveScene()->CreateMeshEntity("New Mesh", handle, assetManager->GetDefaultMaterial());
                // ImportMesh(path);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (dragHover)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRect(
            viewportMin,
            viewportMax,
            IM_COL32(80, 160, 255, 220), // blue highlight
            0.0f,
            0,
            2.0f
        );
    }

    EventBus::Publish(ViewportEvent{
        mousePos.x,
        mousePos.y,
        viewportMin.x,
        viewportMin.y,
        viewportSize.x,
        viewportSize.y,
        hovered
        });

    m_ViewportHovered = hovered;

    ImGui::End();
    ImGui::PopStyleVar(2);
}
