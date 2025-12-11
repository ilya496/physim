#include "Window.h"

#include <iostream>
#include "Event.h"
#include "EventBus.h"

static bool s_WindowInitialized = false;

Window::Window(const WindowProps& props)
    : m_Width(props.width), m_Height(props.height), m_Title(props.title), m_VSync(props.VSync)
{
    Init();
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
}

void Window::Init()
{
    if (!s_WindowInitialized)
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize window!" << '\n';
        }
        s_WindowInitialized = true;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);

    if (!m_Window)
        std::cerr << "Failed to create GLFW window!" << '\n';

    glfwMakeContextCurrent(m_Window);
    SetVSync(m_VSync);

    glfwSetWindowUserPointer(m_Window, this);

    InitCallbacks();
}

void Window::OnUpdate()
{
    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}

void Window::SetTitle(const std::string& title)
{
    m_Title = title;
    glfwSetWindowTitle(m_Window, title.c_str());
}

void Window::SetVSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
    m_VSync = enabled;
}

glm::ivec2 Window::GetFramebufferSize() const
{
    int w, h;
    glfwGetFramebufferSize(m_Window, &w, &h);
    return { w, h };
}

glm::ivec2 Window::GetPosition() const
{
    int x, y;
    glfwGetWindowPos(m_Window, &x, &y);
    return { x, y };
}

void Window::SetPosition(int x, int y)
{
    glfwSetWindowPos(m_Window, x, y);
}

void Window::Minimize()
{
    glfwIconifyWindow(m_Window);
}

void Window::Maximize()
{
    glfwMaximizeWindow(m_Window);
}

void Window::Restore()
{
    glfwRestoreWindow(m_Window);
}

bool Window::IsMaximized() const
{
    return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
}

void Window::MakeContextCurrent()
{
    glfwMakeContextCurrent(m_Window);
}

void Window::DetachContext()
{
    glfwMakeContextCurrent(nullptr);
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(m_Window);
}

void Window::Close()
{
    glfwSetWindowShouldClose(m_Window, true);
}

void Window::SetFullscreen(bool enabled)
{
    if (enabled == m_IsFullscreen)
        return;

    if (enabled)
    {
        m_PreviousMonitor = glfwGetWindowMonitor(m_Window);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(m_Window, monitor,
            0, 0,
            mode->width, mode->height,
            mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(m_Window, nullptr,
            100, 100,
            m_Width, m_Height,
            0);
    }

    m_IsFullscreen = enabled;
}

void Window::InitCallbacks()
{
    // -------- Window Resize --------
    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* wnd, int w, int h)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(wnd);
            // if (!window->m_EventCallback) return;

            WindowResizeEvent ev(w, h);
            // window->m_EventCallback(ev);
            EventBus::Publish(ev);
        });

    // -------- Window Close --------
    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* wnd)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(wnd);
            // if (!window->m_EventCallback) return;

            WindowCloseEvent ev{};
            // window->m_EventCallback(ev);
            EventBus::Publish(ev);
        });

    // -------- Key Events --------
    glfwSetKeyCallback(m_Window, [](GLFWwindow* wnd, int key, int sc, int action, int mods)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(wnd);
            // if (!window->m_EventCallback) return;

            if (action == GLFW_PRESS)
            {
                KeyPressEvent ev(key, false);
                // window->m_EventCallback(ev);
                EventBus::Publish(ev);
            }
            else if (action == GLFW_REPEAT)
            {
                KeyPressEvent ev(key, true);
                // window->m_EventCallback(ev);
                EventBus::Publish(ev);
            }
            else if (action == GLFW_RELEASE)
            {
                KeyReleaseEvent ev(key);
                // window->m_EventCallback(ev);
                EventBus::Publish(ev);
            }
        });

    // -------- Mouse Button Events --------
    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* wnd, int button, int action, int mods)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(wnd);
            // if (!window->m_EventCallback) return;

            if (action == GLFW_PRESS)
            {
                MouseButtonPressEvent ev(button);
                // window->m_EventCallback(ev);
                EventBus::Publish(ev);
            }
            else if (action == GLFW_RELEASE)
            {
                MouseButtonReleaseEvent ev(button);
                // window->m_EventCallback(ev);
                EventBus::Publish(ev);
            }
        });

    // -------- Mouse Move --------
    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* wnd, double x, double y)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(wnd);
            // if (!window->m_EventCallback) return;

            MouseMoveEvent ev((float)x, (float)y);
            // window->m_EventCallback(ev);
            EventBus::Publish(ev);
        });

    // -------- Mouse Scroll --------
    glfwSetScrollCallback(m_Window, [](GLFWwindow* wnd, double x, double y)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(wnd);
            // if (!window->m_EventCallback) return;

            MouseScrollEvent ev((float)x, (float)y);
            // window->m_EventCallback(ev);
            EventBus::Publish(ev);
        });
}
