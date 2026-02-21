#pragma once

#include <string>
#include <functional>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

struct WindowProps
{
    std::string title = "Physim";
    int width = 1920;
    int height = 1080;
    bool VSync = false;
    bool Maximized = true;
};

class Window
{
public:
    Window(const WindowProps& props = {});
    ~Window();

    void OnUpdate();

    void SetTitle(const std::string& title);
    bool IsVSync() const { return m_VSync; }
    void SetVSync(bool enabled);

    glm::ivec2 GetFramebufferSize() const;
    glm::ivec2 GetPosition() const;
    void SetPosition(int x, int y);

    GLFWwindow* GetNativeWindow() const { return m_Window; }

    void Minimize();
    void Maximize();
    void Restore();

    bool IsFullscreen() const { return m_IsFullscreen; }
    void SetFullscreen(bool enabled);
    bool IsMaximized() const;

    void MakeContextCurrent();
    void DetachContext();
    void PollEvents();
    void SwapBuffers();

    void Close();

private:
    void Init();
    void InitCallbacks();

private:
    GLFWwindow* m_Window = nullptr;
    int m_Width, m_Height;
    std::string m_Title;
    bool m_VSync;

    bool m_IsFullscreen = false;
    GLFWmonitor* m_PreviousMonitor = nullptr;

    glm::vec2 m_LastMousePos;
};