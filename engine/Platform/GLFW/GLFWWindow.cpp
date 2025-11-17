#include "GLFWWindow.h"
#include <iostream>

GLFWWindowIMPL::GLFWWindowIMPL(const WindowProps& windowProps) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << '\n';
    }

    // OpenGLContext::setWindowHints();

    m_NativeWindow = glfwCreateWindow(windowProps.width, windowProps.height, (windowProps.title).c_str(), NULL, NULL);
    if (!m_NativeWindow) {
        std::cerr << "Failed to generate GLFW window!" << '\n';
    }

    // m_Context = new OpenGLContext(m_NativeWindow);
    // m_Context->init();

    SetVSync(windowProps.VSync);

    glfwSetWindowUserPointer(m_NativeWindow, this);
    // glfwSetKeyCallback(m_NativeWindow, GLFWKeyCallback);
    // glfwSetMouseButtonCallback(m_NativeWindow, GLFWMouseButtonCallback);
    // glfwSetCursorPosCallback(m_NativeWindow, GLFWMousePositionCallback);
    // glfwSetScrollCallback(m_NativeWindow, GLFWScrollCallback);
    // glfwSetFramebufferSizeCallback(m_NativeWindow, GLFWWindowResizeCallback);
}

GLFWWindowIMPL::~GLFWWindowIMPL() {
    glfwTerminate();
}

void GLFWWindowIMPL::OnUpdate() {
    glfwPollEvents();
    // m_Context->swapBuffers();
}

void GLFWWindowIMPL::SetTitle(const std::string& title) {
    glfwSetWindowTitle(m_NativeWindow, title.c_str());
}

glm::ivec2 GLFWWindowIMPL::GetFramebufferSize() const {
    int width, height;
    glfwGetFramebufferSize(m_NativeWindow, &width, &height);
    return glm::ivec2(width, height);
}

bool GLFWWindowIMPL::IsVSync() const {
    return m_VSync;
}

void GLFWWindowIMPL::SetVSync(bool enabled) {
    m_VSync = enabled;
    glfwSwapInterval(enabled);
}

void* GLFWWindowIMPL::GetNativeWindow() const {
    return m_NativeWindow;
}

void GLFWWindowIMPL::Minimize() {
    glfwIconifyWindow(m_NativeWindow);
}

void GLFWWindowIMPL::Maximize() {
    glfwMaximizeWindow(m_NativeWindow);
}

void GLFWWindowIMPL::Restore() {
    glfwRestoreWindow(m_NativeWindow);
}

bool GLFWWindowIMPL::IsMaximized() const {
    return glfwGetWindowAttrib(m_NativeWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
}

void GLFWWindowIMPL::Close() {
    glfwSetWindowShouldClose(m_NativeWindow, GLFW_TRUE);
}

void GLFWWindowIMPL::SetPosition(int x, int y) {
    glfwSetWindowPos(m_NativeWindow, x, y);
}

glm::ivec2 GLFWWindowIMPL::GetPosition() const {
    int x, y;
    glfwGetWindowPos(m_NativeWindow, &x, &y);
    return glm::ivec2(x, y);
}

void GLFWWindowIMPL::SetFullscreen(bool enabled) {
    if (enabled == m_Fullscreen) {
        return; // already in desired state
    }

    m_Fullscreen = enabled;

    if (enabled) {
        // Save current windowed position and size
        glfwGetWindowPos(m_NativeWindow, &m_WindowedPosX, &m_WindowedPosY);
        glfwGetWindowSize(m_NativeWindow, &m_WindowedWidth, &m_WindowedHeight);

        // Get primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // Switch to fullscreen
        glfwSetWindowMonitor(m_NativeWindow, monitor,
            0, 0, mode->width, mode->height,
            mode->refreshRate);
    }
    else {
        // Switch back to windowed
        glfwSetWindowMonitor(m_NativeWindow, nullptr,
            m_WindowedPosX, m_WindowedPosY,
            m_WindowedWidth, m_WindowedHeight,
            0);
    }
}

bool GLFWWindowIMPL::IsFullscreen() const {
    return m_Fullscreen;
}

// takes a const reference to a function returning void and argument std::shared_ptr<IEvent>
// void GLFWWindowIMPL::setEventCallback(const std::function<void(std::shared_ptr<IEvent>)>& callback) {
//     dispatchEvent = callback;
// }

// #define thisWindow static_cast<GLFWWindowIMPL*>(glfwGetWindowUserPointer(window))

// void GLFWWindowIMPL::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
//     KeyCode k = static_cast<KeyCode>(key);
//     bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
//     bool shift = (mods & GLFW_MOD_SHIFT) != 0;
//     bool alt = (mods & GLFW_MOD_ALT) != 0;
//     bool super = (mods & GLFW_MOD_SUPER) != 0;

//     switch (action) {
//     case GLFW_PRESS:
//         thisWindow->dispatchEvent(std::make_shared<KeyPressEvent>(k, ctrl, shift, alt, super));
//         return;
//     case GLFW_RELEASE:
//         thisWindow->dispatchEvent(std::make_shared<KeyReleaseEvent>(k, ctrl, shift, alt, super));
//         return;
//     case GLFW_REPEAT:
//         thisWindow->dispatchEvent(std::make_shared<KeyRepeatEvent>(k, ctrl, shift, alt, super));
//         return;
//     }
// }

// void GLFWWindowIMPL::GLFWMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
//     thisWindow->dispatchEvent(std::make_shared<MouseMoveEvent>(xpos, ypos));
// }

// void GLFWWindowIMPL::GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
//     MouseCode b = static_cast<MouseCode>(button);

//     switch (action) {
//     case GLFW_PRESS:
//         thisWindow->dispatchEvent(std::make_shared<MouseButtonPressEvent>(button));
//         return;
//     case GLFW_RELEASE:
//         thisWindow->dispatchEvent(std::make_shared<MouseButtonReleaseEvent>(button));
//         return;
//     }
// }

// void GLFWWindowIMPL::GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
//     thisWindow->dispatchEvent(std::make_shared<MouseScrollEvent>(xoffset, yoffset));
// }

// void GLFWWindowIMPL::GLFWWindowResizeCallback(GLFWwindow* window, int width, int height) {
//     thisWindow->dispatchEvent(std::make_shared<WindowResizeEvent>(width, height));
// }

// bool GLFWWindowIMPL::isKeyPressed(KeyCode key) {
//     auto state = glfwGetKey(m_NativeWindow, key);
//     return state == GLFW_PRESS || state == GLFW_REPEAT;
// }

// bool GLFWWindowIMPL::isMouseButtonPressed(MouseCode button) {
//     auto state = glfwGetMouseButton(m_NativeWindow, button);
//     return state == GLFW_PRESS;
// }

// std::pair<float, float> GLFWWindowIMPL::getMousePosition() {
//     double xpos, ypos;
//     glfwGetCursorPos(m_NativeWindow, &xpos, &ypos);
//     return { (float)xpos, (float)ypos };
// }

bool GLFWWindowIMPL::ShouldClose() {
    return glfwWindowShouldClose(m_NativeWindow);
}