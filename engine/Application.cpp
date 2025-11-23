#include "Application.h"

#include <iostream>
#include "Timer.h"

Application::Application(WindowProps windowProps)
{
    m_Window = std::make_unique<Window>(windowProps);

    m_Window->SetEventCallback([this](Event& event) {
        OnEvent(event);
        });

    m_LastFrameTime = glfwGetTime();
}

void Application::Run()
{
    while (m_Running && !m_Window->ShouldClose())
    {
        double now = glfwGetTime();
        Timer::Update(now);

        m_TimeAccumulator += Timer::DeltaTime();
        while (m_TimeAccumulator >= Timer::FixedDeltaTime())
        {
            m_LayerStack.OnFixedUpdate(Timer::FixedDeltaTime());
            m_TimeAccumulator -= Timer::FixedDeltaTime();
        }

        m_LayerStack.OnUpdate(Timer::DeltaTime());

        m_LayerStack.OnRender();
        m_Window->OnUpdate();

        std::cout << "FPS: " << Timer::FPS() << '\n';
        std::cout << "Average Frame Time: " << Timer::AverageFrameTime() << '\n';
    }

    Shutdown();
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    m_LayerStack.PushLayer(std::move(layer));
}

void Application::OnEvent(Event& e)
{
    // std::cout << e.ToString() << '\n';

    if (e.GetType() == EventType::WINDOW_CLOSE_EVENT)
    {
        m_Running = false;
        e.Consume();
    }

    // Forward to LayerStack
    if (!e.IsConsumed())
        m_LayerStack.Broadcast(e);
}