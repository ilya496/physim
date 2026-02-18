#pragma once

#include "Layer.h"
#include "render/Renderer.h"
#include "EventBus.h"

struct Ray
{
    glm::vec3 Origin;
    glm::vec3 Direction;
};

enum class GizmoMode
{
    None,
    Translate,
    Rotate
};

enum class GizmoAxis
{
    Free,
    X,
    Y,
    Z
};

class RenderLayer : public Layer
{
public:
    RenderLayer(uint32_t width, uint32_t height);

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float dt) override;
    virtual void OnRender() override;

private:
    entt::entity PickEntity(float mouseX, float mouseY);
    void CreateFramebuffer(uint32_t width, uint32_t height);
    void ResizeFramebuffer(uint32_t width, uint32_t height);

private:
    std::unique_ptr<Renderer> m_Renderer;
    Camera m_Camera;

    GizmoMode m_GizmoMode = GizmoMode::None;
    GizmoAxis m_GizmoAxis = GizmoAxis::Free;
    bool m_GizmoActive = false;

    glm::vec2 m_GizmoStartMouse{};
    glm::vec3 m_GizmoStartPosition{};
    glm::quat m_GizmoStartRotation{};

    uint32_t m_Framebuffer = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    uint32_t m_Width, m_Height;

    EventBus::Subscription m_ViewportEvent;
    EventBus::Subscription m_MouseScrollEvent;
    EventBus::Subscription m_MousePressEvent;
    EventBus::Subscription m_MouseMoveEvent;
    EventBus::Subscription m_MouseReleaseEvent;

    bool m_MouseDown[8] = {};

    entt::entity m_SelectedEntity = entt::null;

    float m_ViewportX = 0.0f;
    float m_ViewportY = 0.0f;
    float m_ViewportWidth = 0.0f;
    float m_ViewportHeight = 0.0f;
    float m_MouseX = 0.0f;
    float m_MouseY = 0.0f;
    bool m_ViewportHovered = false;

    bool m_PendingResize = false;
    float m_RequestedWidth = 0;
    float m_RequestedHeight = 0;
};