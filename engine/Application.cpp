#include "Application.h"

#include <iostream>
#include "Timer.h"

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

    m_WindowResizeSub =
        EventBus::Subscribe<WindowResizeEvent>(
            [this](const WindowResizeEvent& e)
            {
                // glViewport(0, 0, e.Width, e.Height);
                // std::cout << "Window resized!" << '\n';
            }
        );

    m_LastFrameTime = glfwGetTime();

    m_Window->DetachContext();
    m_RenderThread = std::thread(&Application::RenderThreadFunc, this);
}

Application::~Application()
{
    m_RenderThreadRunning = false;
    if (m_RenderThread.joinable())
        m_RenderThread.join();
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

        m_RenderState = m_SimState;
        m_NewFrameAvailable = true;

        // m_LayerStack.OnRender();
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

void Application::RenderThreadFunc()
{
    m_Window->MakeContextCurrent();

    while (m_RenderThreadRunning)
    {
        if (!m_NewFrameAvailable)
        {
            // Avoid spinning at 100% CPU
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            continue;
        }

        m_NewFrameAvailable = false;

        // 1. Render using m_RenderState
        // Call your layers' render:
        m_LayerStack.OnRender();

        // 2. Swap buffers
        m_Window->SwapBuffers();
    }
}