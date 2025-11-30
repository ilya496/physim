#pragma once

#include "Layer.h"
#include "Window.h"

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

private:
    Window* m_Window = nullptr;
};