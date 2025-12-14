#include "Application.h"

#include <iostream>
#include "Timer.h"
#include "RenderLayer.h"

Application::Application(WindowProps windowProps)
{
    m_Window = std::make_unique<Window>(windowProps);

    m_WindowCloseSub =
        EventBus::Subscribe<WindowCloseEvent>(
            [this](const WindowCloseEvent& e)
            {
                m_Running = false;
            }
        );

    m_LayerStack.PushLayer(std::make_unique<RenderLayer>(800, 600));
    std::cout << "render layer pushed";

    m_LastFrameTime = glfwGetTime();
}


void Application::Run()
{
    while (m_Running)
    {
        m_Window->PollEvents();
        double now = glfwGetTime();
        Timer::Update(now);

        m_TimeAccumulator += Timer::DeltaTime();
        while (m_TimeAccumulator >= m_FixedDeltaTime)
        {
            m_LayerStack.OnFixedUpdate(m_FixedDeltaTime);
            m_TimeAccumulator -= m_FixedDeltaTime;
        }

        m_LayerStack.OnUpdate(Timer::DeltaTime());

        m_LayerStack.OnRender();

        std::cout << "FPS: " << Timer::FPS()
            << "   Frame: " << Timer::AverageFrameTime() << "\n";

        m_Window->SwapBuffers();
    }

    Shutdown();
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    m_LayerStack.PushLayer(std::move(layer));
}