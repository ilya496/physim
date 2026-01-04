#include "Camera.h"

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_FOV(fov), m_Aspect(aspectRatio), m_Near(nearClip), m_Far(farClip)
{
    RecalculateProjection();
    RecalculateView();
}

void Camera::SetViewport(uint32_t width, uint32_t height)
{
    m_Aspect = (float)width / (float)height;
    RecalculateProjection();
}

void Camera::SetPerspective(float fov, float nearClip, float farClip)
{
    m_FOV = fov;
    m_Near = nearClip;
    m_Far = farClip;
    RecalculateProjection();
}

void Camera::SetPosition(const glm::vec3& position)
{
    m_Position = position;
    RecalculateView();
}

void Camera::LookAt(const glm::vec3& target)
{
    m_Target = target;
    m_Forward = glm::normalize(target - m_Position);

    m_Yaw = glm::degrees(atan2(m_Forward.z, m_Forward.x)) - 90.0f;
    m_Pitch = glm::degrees(asin(m_Forward.y));

    RecalculateView();
}

void Camera::Orbit(const glm::vec2& delta)
{
    constexpr float sensitivity = 0.3f;

    m_Yaw += delta.x * sensitivity;
    m_Pitch += delta.y * sensitivity;

    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

    float distance = glm::length(m_Position - m_Target);

    glm::vec3 dir;
    dir.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    dir.y = sin(glm::radians(m_Pitch));
    dir.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    dir = glm::normalize(dir);

    m_Position = m_Target - dir * distance;
    RecalculateView();
}

void Camera::Pan(const glm::vec2& delta)
{
    constexpr float speed = 0.002f;

    glm::vec3 pan =
        -m_Right * delta.x * speed +
        m_Up * delta.y * speed;

    m_Position += pan;
    m_Target += pan;

    RecalculateView();
}

void Camera::Zoom(float delta)
{
    constexpr float speed = 0.1f;

    glm::vec3 dir = glm::normalize(m_Target - m_Position);
    m_Position += dir * delta * speed;

    RecalculateView();
}

void Camera::Rotate(float yawDelta, float pitchDelta)
{
    constexpr float sensitivity = 0.1f;

    m_Yaw += yawDelta * sensitivity;
    m_Pitch += pitchDelta * sensitivity;

    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
    RecalculateView();
}

void Camera::Move(const glm::vec3& direction, float dt)
{
    constexpr float speed = 20.0f;
    m_Position += direction * speed * dt;
    RecalculateView();
}


void Camera::RecalculateView()
{
    m_Forward.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Forward.y = sin(glm::radians(m_Pitch));
    m_Forward.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Forward = glm::normalize(m_Forward);

    m_Right = glm::normalize(glm::cross(m_Forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_Up = glm::normalize(glm::cross(m_Right, m_Forward));

    m_View = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
    m_ViewProjection = m_Projection * m_View;
}

void Camera::RecalculateProjection()
{
    m_Projection = glm::perspective(
        glm::radians(m_FOV),
        m_Aspect,
        m_Near,
        m_Far
    );
    m_ViewProjection = m_Projection * m_View;
}