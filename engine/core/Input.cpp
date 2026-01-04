#include "Input.h"
#include "Application.h"

double Input::s_LastMouseX;
double Input::s_LastMouseY;

bool Input::IsKeyPressed(KeyCode key)
{
    GLFWwindow* window = Application::Get().GetWindow().GetNativeWindow();
    int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS;
}

bool Input::MouseButtonPressed(MouseCode button)
{
    GLFWwindow* window = Application::Get().GetWindow().GetNativeWindow();
    int state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;

}

glm::vec2 Input::GetMousePosition()
{
    GLFWwindow* window = Application::Get().GetWindow().GetNativeWindow();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return { (float)xpos, (float)ypos };
}

glm::vec2 Input::GetMouseDelta()
{
    GLFWwindow* window = Application::Get().GetWindow().GetNativeWindow();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    glm::vec2 delta = { (float)(xpos - s_LastMouseX), (float)(ypos - s_LastMouseY) };

    s_LastMouseX = xpos;
    s_LastMouseY = ypos;

    return delta;
}