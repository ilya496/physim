#pragma once

#include "core/Window.h"
#include "core/Layer.h"
#include "core/EventBus.h"
#include "EditorSettings.h"

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
    void OnRender() override;
private:
    void SetupImGuiFonts(const char* fontPath);
    void BeginDockspace();
    void DrawAssetsPanel();
    void DrawViewport();

    void DrawLauncher();
    void DrawRecentProjects();
    void OpenProject(const std::filesystem::path& path);
    void CreateNewProject();

private:
    Window* m_Window = nullptr;
    EditorState m_State = EditorState::Launcher;

    EditorSettings m_Settings;
    std::filesystem::path m_SelectedProject;
    int m_SelectedProjectIndex = -1;

    uint32_t m_ViewportTexture = 0;
    uint32_t m_ViewportWidth = 0;
    uint32_t m_ViewportHeight = 0;
    EventBus::Subscription m_NewFrameSub;
};