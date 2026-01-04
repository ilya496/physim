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
#include "GizmoMode.h"

enum class EditorState
{
    Launcher,
    Editor
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
    void DrawToolbar();

    void OpenProject(const std::filesystem::path& path);
    void CreateNewProject();

private:
    Window* m_Window = nullptr;
    EditorState m_State = EditorState::Launcher;
    GizmoMode m_GizmoMode = GizmoMode::None;
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
};