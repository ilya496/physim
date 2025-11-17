#pragma once

#include <GLFW/glfw3.h>
#include "Window.h"

class GLFWWindowIMPL : public Window {
public:
    GLFWWindowIMPL(const WindowProps& windowProps);
    ~GLFWWindowIMPL();

    void OnUpdate() override;

    void SetTitle(const std::string& title) override;

    glm::ivec2 GetFramebufferSize() const override;

    bool IsVSync() const override;
    void SetVSync(bool enabled) override;

    void* GetNativeWindow() const override;

    bool IsFullscreen() const override;
    void SetFullscreen(bool enabled) override;

    // Custom titlebar/window control
    void Minimize() override;
    void Maximize() override;
    void Restore() override;
    bool IsMaximized() const override;
    void Close() override;
    void SetPosition(int x, int y) override;
    glm::ivec2 GetPosition() const override;

    ///  input polling
    // bool isKeyPressed(KeyCode key) override;
    // bool isMouseButtonPressed(MouseCode) override;
    // glm::vec2 getMousePosition() override;
    // void setEventCallback(const std::function<void(std::shared_ptr<IEvent>)>& callback) override;

    bool ShouldClose() override;

private:
    GLFWwindow* m_NativeWindow;
    // OpenGLContext* m_Context;

    // std::function<void(std::shared_ptr<IEvent>)> dispatchEvent;

    bool m_Fullscreen = false, m_VSync = false;
    int m_WindowedPosX, m_WindowedPosY, m_WindowedWidth, m_WindowedHeight; // cache windowed position (during fullscreen)

    /// These are callback methods called by GLFW when an event occurs.
    /// They are static methods because GLFW requires them to be static. The method gets the EventDispatcher
    /// object of the current instance (self) from the GLFW window user pointer to perform actions as if the function was not static.
    // static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    // static void GLFWMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
    // static void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    // static void GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    // static void GLFWWindowResizeCallback(GLFWwindow* window, int width, int height);
};