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

    m_PlayButtonIcon = Texture::Create("../editor/icons/play-button.png");
    m_PauseButtonIcon = Texture::Create("../editor/icons/pause-button.png");;
    m_StopButtonIcon = Texture::Create("../editor/icons/stop-button.png");;
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

void EditorLayer::DrawViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::Begin("Viewport", nullptr,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

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

    // ===============================
// Viewport top-center toolbar
// ===============================

    const float buttonSize = 25.0f;
    const float padding = 6.0f;
    const float toolbarHeight = buttonSize + padding * 2.0f;
    const float spacing = 6.0f;

    float totalWidth =
        buttonSize * 3.0f +
        spacing * 2.0f +
        padding * 2.0f;

    ImVec2 toolbarMin = {
        viewportMin.x + (viewportSize.x - totalWidth) * 0.5f,
        viewportMin.y + 8.0f
    };

    ImVec2 toolbarMax = {
        toolbarMin.x + totalWidth,
        toolbarMin.y + toolbarHeight
    };

    // Background
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(
        toolbarMin,
        toolbarMax,
        IM_COL32(25, 25, 25, 220),
        6.0f
    );

    // Border
    dl->AddRect(
        toolbarMin,
        toolbarMax,
        IM_COL32(60, 60, 60, 255),
        6.0f
    );

    // Buttons
    ImGui::SetCursorScreenPos({
        toolbarMin.x + padding,
        toolbarMin.y + padding
        });

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    if (ImGui::ImageButton("##PlayButton", (ImTextureID)m_PlayButtonIcon->GetRendererID(), ImVec2(buttonSize, buttonSize)))
    {
        m_SceneController.Play();
    }

    ImGui::SameLine(0.0f, spacing);

    if (ImGui::ImageButton("##PauseButton", (ImTextureID)m_PauseButtonIcon->GetRendererID(), ImVec2(buttonSize, buttonSize)))
    {
        m_SceneController.Pause();
    }

    ImGui::SameLine(0.0f, spacing);

    if (ImGui::ImageButton("##StopButton", (ImTextureID)m_StopButtonIcon->GetRendererID(), ImVec2(buttonSize, buttonSize)))
    {
        m_SceneController.Stop();
    }

    ImGui::PopStyleVar(2);

    // ========================================
    // Simulation Info (Top Right Overlay)
    // ========================================

    SimulationState simState = m_SceneController.GetState();

    const char* stateStr = "Stopped";
    switch (simState)
    {
    case SimulationState::Running: stateStr = "Running"; break;
    case SimulationState::Paused:  stateStr = "Paused";  break;
    case SimulationState::Stopped: stateStr = "Stopped"; break;
    }

    int currentFrame = m_SceneController.GetCurrentFrameIndex();
    int totalFrames = m_SceneController.GetTotalFrames();

    // Build info string
    char infoBuffer[128];
    snprintf(infoBuffer, sizeof(infoBuffer),
        "State: %s\nFrame: %d / %d",
        stateStr,
        totalFrames > 0 ? currentFrame : 0,
        totalFrames > 0 ? totalFrames - 1 : 0
    );

    // Measure text size for proper right alignment
    ImVec2 textSize = ImGui::CalcTextSize(infoBuffer);

    // Padding from edge
    const float textPadding = 10.0f;

    ImVec2 textPos = {
        viewportMax.x - textSize.x - textPadding,
        viewportMin.y + textPadding
    };

    // Background panel
    ImVec2 bgMin = {
        textPos.x - 8.0f,
        textPos.y - 6.0f
    };

    ImVec2 bgMax = {
        textPos.x + textSize.x + 8.0f,
        textPos.y + textSize.y + 6.0f
    };

    dl->AddRectFilled(
        bgMin,
        bgMax,
        IM_COL32(20, 20, 20, 200),
        6.0f
    );

    dl->AddRect(
        bgMin,
        bgMax,
        IM_COL32(60, 60, 60, 255),
        6.0f
    );

    dl->AddText(
        textPos,
        IM_COL32(255, 255, 255, 255),
        infoBuffer
    );

    ImGui::SetCursorScreenPos(viewportMin);
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
        // ImDrawList* drawList = ImGui::GetWindowDrawList();

        dl->AddRect(
            viewportMin,
            viewportMax,
            IM_COL32(80, 160, 255, 220), // blue highlight
            0.0f,
            0,
            2.0f
        );
    }

    static bool showAddMeshPopup = false;

    if (hovered && Input::IsKeyPressed(KeyCode::A) && Input::IsKeyPressed(KeyCode::LeftShift))
    {
        showAddMeshPopup = true;
    }

    if (showAddMeshPopup)
    {
        ImGui::OpenPopup("Add Mesh");
    }

    if (ImGui::BeginPopupModal("Add Mesh", &showAddMeshPopup))
    {
        if (ImGui::MenuItem("Cube"))
        {
            showAddMeshPopup = false;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("UV Sphere"))
        {
            showAddMeshPopup = false;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Ico Sphere"))
        {
            showAddMeshPopup = false;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Plane"))
        {
            showAddMeshPopup = false;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Cylinder"))
        {
            showAddMeshPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
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
