#pragma once

#include "Window.h"

class Application
{
public:
    Application(WindowProps windowProps = WindowProps{});
    virtual ~Application() = default;

public:
    void Run();

protected:
    virtual void Shutdown();

    std::shared_ptr<Window> m_Window;
};

Application* CreateApplication();