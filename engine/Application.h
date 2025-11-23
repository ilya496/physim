#pragma once

#include <memory>

#include "Window.h"
#include "Event.h"
#include "LayerStack.h"

class Application
{
public:
    Application(WindowProps windowProps = WindowProps{});
    virtual ~Application() = default;

    void Run();

    void PushLayer(std::unique_ptr<Layer> layer);

protected:
    virtual void Shutdown() {}
    virtual void OnEvent(Event& event);

protected:
    std::unique_ptr<Window> m_Window;
    LayerStack m_LayerStack;
    bool m_Running = true;

    float m_DeltaTime = 0.0f;
    float m_FixedDeltaTime = 1.0f / 60.0f;   // 60 Hz physics
    float m_TimeAccumulator = 0.0f;

    double m_LastFrameTime = 0.0;
    double m_TotalTime = 0.0;
};

Application* CreateApplication();