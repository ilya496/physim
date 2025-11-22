#include "Application.h"

Application::Application(WindowProps windowProps)
{
    m_Window = std::make_unique<Window>(windowProps);

    m_Window->SetEventCallback([this](Event& event) {
        OnEvent(event);
        });
}

void Application::Run()
{
    while (m_Running && !m_Window->ShouldClose())
    {
        m_LayerStack.OnUpdate();
        m_Window->OnUpdate();
    }

    Shutdown();
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    m_LayerStack.PushLayer(std::move(layer));
}

void Application::OnEvent(Event& e)
{
    if (e.GetType() == EventType::WINDOW_CLOSE_EVENT)
    {
        m_Running = false;
        e.Consume();
    }

    // Forward to LayerStack
    if (!e.IsConsumed())
        m_LayerStack.Broadcast(e);
}