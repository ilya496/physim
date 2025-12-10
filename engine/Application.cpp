#include "Application.h"

#include <iostream>
#include "Timer.h"
#include "asset/AssetManager.h"

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

    m_LastFrameTime = glfwGetTime();
    AssetManager man;
    man.ImportAsset("../../brickwall.jpg");
    man.SerializeAssetRegistry();
}


void Application::Run()
{
    while (m_Running)
    {
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
        m_Window->PollEvents();

        std::cout << "FPS: " << Timer::FPS()
            << "   Frame: " << Timer::AverageFrameTime() << "\n";
    }

    Shutdown();
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    m_LayerStack.PushLayer(std::move(layer));
}