#pragma once

#include <memory>

#include "Window.h"
#include "LayerStack.h"
#include "EventBus.h"

class Application
{
public:
    Application(WindowProps windowProps = WindowProps{});
    virtual ~Application() = default;

    void Run();
    void PushLayer(std::unique_ptr<Layer> layer);

    Window& GetWindow() { return *m_Window; }

    static Application& Get() { return *s_Instance; }

protected:
    virtual void Shutdown() {}

    std::unique_ptr<Window> m_Window;
    LayerStack m_LayerStack;

    float m_DeltaTime = 0.0f;
    float m_FixedDeltaTime = 1.0f / 60.0f;
    float m_TimeAccumulator = 0.0f;
    double m_LastFrameTime = 0.0;
    double m_TotalTime = 0.0;

private:
    static Application* s_Instance;
    bool m_Running = true;

    EventBus::Subscription m_WindowCloseSub;
    EventBus::Subscription m_WindowResizeSub;
};

Application* CreateApplication();
