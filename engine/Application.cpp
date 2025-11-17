#include "Application.h"

Application::Application(WindowProps windowProps)
{
    m_Window = Window::Create(windowProps);
}

void Application::Run()
{
    while (!m_Window->ShouldClose())
    {
        m_Window->OnUpdate();
    }
    Shutdown();
}

void Application::Shutdown() {}