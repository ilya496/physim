#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(
        float fov = 45.0f,
        float aspectRatio = 16.0f / 9.0f,
        float nearClip = 0.1f,
        float farClip = 1000.0f
    );

    const glm::mat4& GetView() const { return m_View; }
    const glm::mat4& GetProjection() const { return m_Projection; }
    const glm::mat4& GetViewProjection() const { return m_ViewProjection; }

    void SetViewport(uint32_t width, uint32_t height);
    void SetPerspective(float fovDegrees, float nearClip, float farClip);

    void SetPosition(const glm::vec3& position);
    void LookAt(const glm::vec3& target);

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetForward() const { return m_Forward; }
    const glm::vec3& GetRight() const { return m_Right; }
    const glm::vec3& GetUp() const { return m_Up; }

    // editor controls
    void Orbit(const glm::vec2& delta);
    void Pan(const glm::vec2& delta);
    void Zoom(float delta);

    // fly controls
    void Move(const glm::vec3& direction, float deltaTime);
    void Rotate(float yawDelta, float pitchDelta);

private:
    void RecalculateView();
    void RecalculateProjection();

private:
    glm::vec3 m_Position{ 0.0f, 0.0f, 5.0f };
    glm::vec3 m_Target{ 0.0f, 0.0f, 0.0f };

    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;

    glm::vec3 m_Forward{ 0.0f, 0.0f, -1.0f };
    glm::vec3 m_Right{ 1.0f, 0.0f, 0.0f };
    glm::vec3 m_Up{ 0.0f, 1.0f, 0.0f };

    // projection
    float m_FOV = 45.0f;
    float m_Aspect = 16.0f / 9.0f;
    float m_Near = 0.1f;
    float m_Far = 1000.0f;

    // cached matrices
    glm::mat4 m_View{ 1.0f };
    glm::mat4 m_Projection{ 1.0f };
    glm::mat4 m_ViewProjection{ 1.0f };
};
