#include "EditorLayer.h"

#include "core/EventBus.h"
#include "project/Project.h"
#include "utils/FileDialog.h"
#include <iostream>
#include "core/Input.h"
#include "EditorContext.h"
#include <algorithm>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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
    m_InspectorPanel = std::make_unique<InspectorPanel>(&m_SceneController);
    m_AssetPanel = std::make_unique<AssetPanel>();
    m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>();

    m_NewFrameSub = EventBus::Subscribe<NewFrameRenderedEvent>(
        [this](const NewFrameRenderedEvent& e)
        {
            m_ViewportTexture = e.ColorAttachment;
            m_ViewportWidth = e.Width;
            m_ViewportHeight = e.Height;

            if (!m_ExportRunning || !e.PixelData)
                return;

            const int frame = m_SceneController.GetCurrentFrameIndex();

            SavePixelsToPNG(*e.PixelData, e.Width, e.Height, frame);

            m_ExportProgress = float(frame) / float(m_ExportEndFrame);

            if (frame >= m_ExportEndFrame)
            {
                // Done — restore stopped state
                m_SceneController.Stop();
                m_ExportRunning = false;
                m_ExportProgress = 1.0f;
                m_PendingExportStep = false;
                return;
            }

            // Signal OnUpdate to advance one frame next tick
            m_PendingExportStep = true;
        }
    );

    m_PlayButtonIcon = Texture::Create("../editor/icons/play-button.png");
    m_PauseButtonIcon = Texture::Create("../editor/icons/pause-button.png");
    m_StopButtonIcon = Texture::Create("../editor/icons/stop-button.png");
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

    if (m_PendingExportStep)
    {
        m_PendingExportStep = false;
        // StepFrame only works when Paused, which is the state we set in StartExport
        m_SceneController.StepFrame(1);
        EventBus::Publish(RequestFrameCaptureEvent{ true });
    }
    else if (!m_ExportRunning)
    {
        if (Input::IsKeyPressed(KeyCode::LeftControl))
        {
            if (Input::IsKeyPressed(KeyCode::S))
                Project::SaveActive(Project::GetActiveProjectName());

            if (Input::IsKeyPressed(KeyCode::O))
            {
                auto path = FileDialog::OpenFile("Physim Project", "physim");
                if (!path.empty())
                    OpenProject(path);
            }
        }

        if (m_State == EditorState::Editor)
            HandleSimulationShortcuts();
    }

    BeginDockspace();
}

void EditorLayer::OnFixedUpdate(float dt)
{
    if (m_ExportRunning)
        return;

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
                Project::SaveActive(Project::GetActiveProjectName());

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

            if (ImGui::MenuItem("Export Simulation", "Ctrl+E"))
            {
                if (Project::GetActive() && Project::GetActive()->GetActiveScene())
                    m_RequestOpenExportPopup = true;
            }

            if (ImGui::MenuItem("Exit"))
                EventBus::Publish(WindowCloseEvent{});

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    DrawExportPopup();

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

    DrawToolbar(viewportMin, viewportSize);
    DrawSimulationInfo(viewportMin, viewportMax);

    ImGui::SetCursorScreenPos(viewportMin);
    ImGui::InvisibleButton("##ViewportDropTarget", viewportSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight);

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
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (dragHover)
    {
        ImGui::GetWindowDrawList()->AddRect(
            viewportMin,
            viewportMax,
            IM_COL32(80, 160, 255, 220),
            0.0f, 0, 2.0f
        );
    }

    ImGui::PopStyleVar(2);

    EventBus::Publish(ViewportEvent{
        mousePos.x, mousePos.y,
        viewportMin.x, viewportMin.y,
        viewportSize.x, viewportSize.y,
        hovered
        });

    m_ViewportHovered = hovered;

    if (hovered &&
        Input::IsKeyPressed(KeyCode::C) &&
        Input::IsKeyPressed(KeyCode::LeftShift))
    {
        ImGui::OpenPopup("AddMeshContextMenu");
    }

    if (ImGui::BeginPopup("AddMeshContextMenu"))
    {
        ImGui::SeparatorText("Add Mesh");
        if (ImGui::MenuItem("Cube"))
        {
            auto project = Project::GetActive();
            auto assetManager = project->GetAssetManager();
            project->GetActiveScene()->CreateMeshEntity(
                "New Mesh",
                assetManager->GetDefaultMesh(MeshPrimitive::CUBE),
                assetManager->GetDefaultMaterial()
            );
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("UV Sphere"))
        {
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("Plane"))
        {
            auto project = Project::GetActive();
            auto assetManager = project->GetAssetManager();
            project->GetActiveScene()->CreateMeshEntity(
                "New Mesh",
                assetManager->GetDefaultMesh(MeshPrimitive::PLANE),
                assetManager->GetDefaultMaterial()
            );
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void EditorLayer::DrawToolbar(
    const ImVec2& viewportMin,
    const ImVec2& viewportSize)
{
    constexpr float ButtonSize = 25.0f;
    constexpr float Padding = 6.0f;
    constexpr float Spacing = 6.0f;
    constexpr float CornerRounding = 6.0f;
    constexpr float FrameRounding = 4.0f;
    constexpr float TopOffset = 4.0f;
    constexpr int ButtonCount = 5;

    const float toolbarHeight = ButtonSize + Padding * 2.0f;
    const float totalWidth =
        (ButtonSize * ButtonCount) +
        (Spacing * (ButtonCount - 1)) +
        (Padding * 2.0f);

    const ImVec2 toolbarMin = {
        viewportMin.x + (viewportSize.x - totalWidth) * 0.5f,
        viewportMin.y + TopOffset
    };
    const ImVec2 toolbarMax = {
        toolbarMin.x + totalWidth,
        toolbarMin.y + toolbarHeight
    };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(toolbarMin, toolbarMax, IM_COL32(25, 25, 25, 220), CornerRounding);
    drawList->AddRect(toolbarMin, toolbarMax, IM_COL32(60, 60, 60, 255), CornerRounding);

    ImGui::SetCursorScreenPos({ toolbarMin.x + Padding, toolbarMin.y + Padding });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FrameRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    const ImVec2 size(ButtonSize, ButtonSize);

    if (ImGui::Button("<", size)) m_SceneController.StepFrame(-1);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step Back (Left Arrow)");
    ImGui::SameLine(0.0f, Spacing);

    if (ImGui::ImageButton("##PlayButton", (ImTextureID)m_PlayButtonIcon->GetRendererID(), size))
        m_SceneController.Play();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play (Ctrl + P)");
    ImGui::SameLine(0.0f, Spacing);

    if (ImGui::ImageButton("##PauseButton", (ImTextureID)m_PauseButtonIcon->GetRendererID(), size))
        m_SceneController.TogglePause();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause / Resume (Space)");
    ImGui::SameLine(0.0f, Spacing);

    if (ImGui::ImageButton("##StopButton", (ImTextureID)m_StopButtonIcon->GetRendererID(), size))
        m_SceneController.Stop();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop (Ctrl + .)");
    ImGui::SameLine(0.0f, Spacing);

    if (ImGui::Button(">", size)) m_SceneController.StepFrame(1);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step Forward (Right Arrow)");

    ImGui::PopStyleVar(2);
}

void EditorLayer::DrawSimulationInfo(
    const ImVec2& viewportMin,
    const ImVec2& viewportMax)
{
    const SimulationState simState = m_SceneController.GetState();

    const char* stateStr = "Unknown";
    switch (simState)
    {
    case SimulationState::Stopped: stateStr = "Stopped"; break;
    case SimulationState::Running: stateStr = "Running"; break;
    case SimulationState::Paused:  stateStr = "Paused";  break;
    }

    const int totalFrames = m_SceneController.GetTotalFrames();
    const int currentFrame = m_SceneController.GetCurrentFrameIndex();
    const int displayFrame = (totalFrames > 0) ? currentFrame : 0;
    const int displayTotal = (totalFrames > 0) ? totalFrames - 1 : 0;

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer),
        "State: %s\nFrame: %d / %d",
        stateStr, displayFrame, displayTotal);

    constexpr float OuterPadding = 10.0f;
    constexpr float InnerPaddingX = 8.0f;
    constexpr float InnerPaddingY = 6.0f;
    constexpr float CornerRounding = 6.0f;

    const ImVec2 textSize = ImGui::CalcTextSize(buffer);
    const ImVec2 textPos = {
        viewportMax.x - textSize.x - OuterPadding,
        viewportMin.y + OuterPadding
    };
    const ImVec2 bgMin = { textPos.x - InnerPaddingX, textPos.y - InnerPaddingY };
    const ImVec2 bgMax = { textPos.x + textSize.x + InnerPaddingX, textPos.y + textSize.y + InnerPaddingY };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(bgMin, bgMax, IM_COL32(20, 20, 20, 200), CornerRounding);
    drawList->AddRect(bgMin, bgMax, IM_COL32(60, 60, 60, 255), CornerRounding);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), buffer);
}

void EditorLayer::HandleSimulationShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    const bool ctrl = io.KeyCtrl;
    const SimulationState state = m_SceneController.GetState();

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_P, false))
    {
        if (state == SimulationState::Stopped || state == SimulationState::Paused)
            m_SceneController.Play();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Period, false))
    {
        if (state != SimulationState::Stopped)
            m_SceneController.Stop();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        if (state == SimulationState::Running || state == SimulationState::Paused)
            m_SceneController.TogglePause();
    }

    if (state != SimulationState::Running)
    {
        static float leftArrowHeldTime = 0.0f;
        static float rightArrowHeldTime = 0.0f;
        constexpr float HoldDelay = 0.2f;
        constexpr float RepeatInterval = 0.05f;

        if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
        {
            rightArrowHeldTime += io.DeltaTime;
            if (rightArrowHeldTime >= HoldDelay &&
                std::fmod(rightArrowHeldTime - HoldDelay, RepeatInterval) < io.DeltaTime)
                m_SceneController.StepFrame(1);
        }
        else rightArrowHeldTime = 0.0f;

        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
        {
            leftArrowHeldTime += io.DeltaTime;
            if (leftArrowHeldTime >= HoldDelay &&
                std::fmod(leftArrowHeldTime - HoldDelay, RepeatInterval) < io.DeltaTime)
                m_SceneController.StepFrame(-1);
        }
        else leftArrowHeldTime = 0.0f;
    }
}

void EditorLayer::DrawExportPopup()
{
    if (m_RequestOpenExportPopup)
    {
        ImGui::OpenPopup("Export Simulation");
        m_RequestOpenExportPopup = false;
    }

    if (ImGui::BeginPopupModal("Export Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const int totalFrames = m_SceneController.GetTotalFrames();
        const float fixedDt = m_SceneController.GetFixedDeltaTime();
        const float duration = totalFrames > 0 ? totalFrames * fixedDt : 0.0f;

        ImGui::Text("Simulation Information");
        ImGui::Separator();
        ImGui::Text("Total Frames: %d", totalFrames);
        ImGui::Text("Duration: %.2f seconds", duration);
        ImGui::Text("FPS: %.2f", 1.0f / fixedDt);

        ImGui::Spacing();
        ImGui::SeparatorText("Export Range");
        ImGui::InputInt("End Frame", &m_ExportEndFrame);
        m_ExportEndFrame = std::clamp(m_ExportEndFrame, 0, totalFrames - 1);

        ImGui::Spacing();
        ImGui::SeparatorText("Output");

        if (ImGui::Button("Select Output Folder"))
        {
            auto folder = FileDialog::SelectFolder("Export Location");
            if (!folder.empty())
                m_ExportPath = folder;
        }

        if (!m_ExportPath.empty())
            ImGui::TextWrapped("Output: %s", m_ExportPath.string().c_str());

        ImGui::Spacing();
        ImGui::Separator();

        bool valid = !m_ExportPath.empty();
        if (!valid) ImGui::BeginDisabled();

        if (ImGui::Button("Start Export", ImVec2(120, 0)))
        {
            StartExport();
            ImGui::CloseCurrentPopup();
        }

        if (!valid) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    if (m_ExportRunning || m_ExportProgress >= 1.0f)
        ImGui::OpenPopup("Exporting...");

    if (ImGui::BeginPopupModal("Exporting...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Exporting simulation...");
        ImGui::Separator();
        ImGui::ProgressBar(m_ExportProgress, ImVec2(300, 0));
        ImGui::Text("%.1f%%", m_ExportProgress * 100.0f);

        if (!m_ExportRunning)
        {
            if (ImGui::Button("Close"))
            {
                m_ExportProgress = 0.0f;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void EditorLayer::StartExport()
{
    m_ExportRunning = true;
    m_ExportProgress = 0.0f;
    m_PendingExportStep = false;

    // Play then immediately pause — this puts the controller into Paused state.
    // StepFrame only advances the frame when Paused, NOT when Stopped.
    // This is the critical fix: Stop() was preventing StepFrame from working.
    m_SceneController.Stop();
    m_SceneController.SetFrame(0);
    m_SceneController.Play();
    m_SceneController.TogglePause(); // now state == Paused, StepFrame will work

    // Request capture of frame 0
    EventBus::Publish(RequestFrameCaptureEvent{ true });
}

void EditorLayer::SavePixelsToPNG(
    const std::vector<uint8_t>& pixels,
    uint32_t width,
    uint32_t height,
    int frameIndex)
{
    std::vector<uint8_t> flipped = pixels;
    FlipImageVertically(flipped.data(), width, height);

    std::filesystem::path output =
        m_ExportPath / ("frame_" + PadFrame(frameIndex) + ".png");

    stbi_write_png(
        output.string().c_str(),
        width, height, 4,
        flipped.data(),
        width * 4
    );
}

std::string EditorLayer::PadFrame(int frame)
{
    std::stringstream ss;
    ss << std::setw(5) << std::setfill('0') << frame;
    return ss.str();
}

void EditorLayer::FlipImageVertically(
    uint8_t* data,
    uint32_t width,
    uint32_t height)
{
    const uint32_t stride = width * 4;
    std::vector<uint8_t> row(stride);

    for (uint32_t y = 0; y < height / 2; y++)
    {
        uint8_t* row1 = data + y * stride;
        uint8_t* row2 = data + (height - 1 - y) * stride;
        memcpy(row.data(), row1, stride);
        memcpy(row1, row2, stride);
        memcpy(row2, row.data(), stride);
    }
}