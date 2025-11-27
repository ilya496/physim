#pragma once

#include <memory>
#include "Layer.h"
#include "Window.h"

class EditorLayer : public Layer
{
public:
    EditorLayer(Window* window);
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(float dt) override;
    void OnRender() override;
private:
    void SetupImGuiFonts(const char* fontPath);
    void ProcessInput(float dt);

private:
    Window* m_Window = nullptr;
};