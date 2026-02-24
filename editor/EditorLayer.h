#pragma once

#include "core/Window.h"
#include "core/Layer.h"
#include "core/EventBus.h"
#include "EditorSettings.h"
#include "LauncherPanel.h"
#include "InspectorPanel.h"
#include "AssetPanel.h"
#include "SceneHierarchyPanel.h"
#include "scene/SceneController.h"

enum class EditorState
{
    Launcher,
    Editor
};

enum class ExportMode
{
    None,
    Running
};

class EditorLayer : public Layer
{
public:
    EditorLayer(Window* window);
    ~EditorLayer() override;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(float dt) override;
    void OnFixedUpdate(float dt) override;
    void OnRender() override;
private:
    void SetupImGuiFonts(const char* fontPath);
    void BeginDockspace();
    void DrawViewport();
    void DrawSimulationInfo(const ImVec2& viewportMin, const ImVec2& viewportMax);
    void DrawToolbar(const ImVec2& viewportMin, const ImVec2& viewportSize);
    void HandleSimulationShortcuts();

    void OpenProject(const std::filesystem::path& path);
    void CreateNewProject();

    void DrawExportPopup();
    void StartExport();
    std::string PadFrame(int frame);
    void SavePixelsToPNG(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, int currentFrame);

    void FlipImageVertically(uint8_t* data, uint32_t width, uint32_t height);

private:
    Window* m_Window = nullptr;
    EditorState m_State = EditorState::Launcher;
    bool m_ViewportHovered = false;

    EditorSettings m_Settings;
    std::unique_ptr<LauncherPanel> m_LauncherPanel;
    std::unique_ptr<InspectorPanel> m_InspectorPanel;
    std::unique_ptr<AssetPanel> m_AssetPanel;
    std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;

    SceneController m_SceneController;

    uint32_t m_ViewportTexture = 0;
    uint32_t m_ViewportWidth = 0;
    uint32_t m_ViewportHeight = 0;
    EventBus::Subscription m_NewFrameSub;

    std::shared_ptr<Texture> m_PlayButtonIcon;
    std::shared_ptr<Texture> m_PauseButtonIcon;
    std::shared_ptr<Texture> m_StopButtonIcon;

    ExportMode m_ExportMode = ExportMode::None;
    int m_ExportEndFrame = 0;

    float m_ExportProgress = 0.0f;      // 0.0 → 1.0
    bool m_ExportRunning = false;

    int m_CurrentExportFrame = 0;
    std::filesystem::path m_ExportPath;
    bool m_RequestOpenExportPopup = false;
    bool m_PendingExportStep = false;
};