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
};

Application* CreateApplication();