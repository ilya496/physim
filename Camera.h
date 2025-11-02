#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};
enum Camera_Mode
{
    MODE_BLENDER,
    MODE_FLY
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
    // Camera attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Euler angles
    float Yaw;
    float Pitch;

    // Options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // Mouse state
    bool firstMouse = true;
    float lastX = 400, lastY = 300;

    // Camera mode
    Camera_Mode Mode = MODE_BLENDER;

    glm::vec3 Target = glm::vec3(0.0f);

    // Constructor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
                                                   MovementSpeed(SPEED),
                                                   MouseSensitivity(SENSITIVITY),
                                                   Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // View matrix
    glm::mat4 GetViewMatrix() { return glm::lookAt(Position, Position + Front, Up); }

    // Keyboard movement
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // Mouse rotation
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMousePan(float xoffset, float yoffset)
    {
        // Distance-based pan speed
        float distance = glm::length(Position - Target);
        float panSpeed = distance * 0.002f; // tweak 0.002f for sensitivity
        Position -= Right * xoffset * panSpeed;
        Position -= Up * yoffset * panSpeed;
        Target -= Right * xoffset * panSpeed;
        Target -= Up * yoffset * panSpeed;
    }

    void ProcessMouseScroll(float yoffset)
    {
        // Distance-based zoom speed
        glm::vec3 direction = glm::normalize(Position - Target);
        float distance = glm::length(Position - Target);
        float zoomSpeed = distance * 0.1f; // tweak 0.1f for sensitivity

        Position += -direction * (float)yoffset * zoomSpeed;
    }

    // ---------------------------
    // Input handling integrated
    // ---------------------------

    void HandleMouseMovement(GLFWwindow *window, double xpos, double ypos)
    {
        if (Mode == MODE_BLENDER)
        {
            // Blender-style: middle mouse only
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) != GLFW_PRESS)
            {
                firstMouse = true;
                return;
            }

            if (firstMouse)
            {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }

            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;

            lastX = xpos;
            lastY = ypos;

            // Shift + middle = pan
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            {
                ProcessMousePan(xoffset, yoffset);
            }
            else
            {
                ProcessMouseMovement(xoffset, yoffset);
            }
        }
        else if (Mode == MODE_FLY)
        {
            // Fly camera: free look with mouse always
            if (firstMouse)
            {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }

            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;

            lastX = xpos;
            lastY = ypos;

            ProcessMouseMovement(xoffset, yoffset);
        }
    }

    void HandleScroll(double xoffset, double yoffset)
    {
        if (Mode == MODE_BLENDER)
        {
            ProcessMouseScroll(yoffset);
        }
        else if (Mode == MODE_FLY)
        {
            Zoom -= (float)yoffset;
            if (Zoom < 1.0f)
                Zoom = 1.0f;
            if (Zoom > 45.0f)
                Zoom = 45.0f;
        }
    }

private:
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    void rotateAroundTarget(float xoffset, float yoffset)
    {
        // Convert offset to angle
        xoffset *= 0.3f; // tweak rotation sensitivity
        yoffset *= 0.3f;

        glm::vec3 direction = Position - Target;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-xoffset), Up);
        direction = glm::vec3(rotation * glm::vec4(direction, 1.0f));

        glm::vec3 right = glm::normalize(glm::cross(direction, Up));
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-yoffset), right);
        direction = glm::vec3(rotation * glm::vec4(direction, 1.0f));

        Position = Target + direction;
        updateCameraVectors();
    }
};
