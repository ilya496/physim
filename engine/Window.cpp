#include "Window.h"
#include "Platform/GLFW/GLFWWindow.h"

std::shared_ptr<Window> Window::Create(WindowProps windowProps = WindowProps{})
{
    return std::make_shared<GLFWWindowIMPL>(windowProps);
}