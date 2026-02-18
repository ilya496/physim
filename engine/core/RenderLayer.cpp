#include "RenderLayer.h"

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include "EventBus.h"
#include "project/Project.h"
#include "scene/Entity.h"
#include "render/RenderTarget.h"
#include "physics/AABB.h"
#include "EditorContext.h"
#include "Input.h"

inline Ray ScreenPointToRay(
    float mouseX,
    float mouseY,
    float viewportWidth,
    float viewportHeight,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::vec3& cameraPos)
{
    // ndc
    float x = (2.0f * mouseX) / viewportWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / viewportHeight;
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    // view space
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // world space
    glm::vec3 rayWorld =
        glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    return { cameraPos, rayWorld };
}

bool RayIntersectsAABB(
    const Ray& ray,
    const AABB& aabb,
    const glm::mat4& model,
    float& outDistance)
{
    // ray to local space
    glm::mat4 invModel = glm::inverse(model);

    glm::vec3 rayOriginLocal =
        glm::vec3(invModel * glm::vec4(ray.Origin, 1.0f));

    glm::vec3 rayDirLocal =
        glm::normalize(glm::vec3(invModel * glm::vec4(ray.Direction, 0.0f)));

    float tMin = 0.0f;
    float tMax = 1e6f;

    for (int i = 0; i < 3; i++)
    {
        if (std::abs(rayDirLocal[i]) < 1e-6f)
        {
            if (rayOriginLocal[i] < aabb.Min[i] ||
                rayOriginLocal[i] > aabb.Max[i])
                return false;
        }
        else
        {
            float invD = 1.0f / rayDirLocal[i];
            float t0 = (aabb.Min[i] - rayOriginLocal[i]) * invD;
            float t1 = (aabb.Max[i] - rayOriginLocal[i]) * invD;

            if (t0 > t1) std::swap(t0, t1);

            tMin = glm::max(tMin, t0);
            tMax = glm::min(tMax, t1);

            if (tMax < tMin)
                return false;
        }
    }

    outDistance = tMin;
    return true;
}


entt::entity RenderLayer::PickEntity(float mouseX, float mouseY)
{
    auto scene = Project::GetActive()->GetActiveScene();
    auto& registry = scene->GetRegistry();

    float localX = mouseX - m_ViewportX;
    float localY = mouseY - m_ViewportY;

    if (localX < 0 || localY < 0 ||
        localX > m_ViewportWidth ||
        localY > m_ViewportHeight)
    {
        return entt::null;
    }

    Ray ray = ScreenPointToRay(
        localX,
        localY,
        m_ViewportWidth,
        m_ViewportHeight,
        m_Camera.GetProjection(),
        m_Camera.GetView(),
        m_Camera.GetPosition()
    );

    entt::entity closest = entt::null;
    float closestDist = std::numeric_limits<float>::max();

    auto view = registry.view<TransformComponent, MeshRenderComponent>();

    for (auto [entity, transform, mr] : view.each())
    {
        auto mesh = AssetManager::GetAsset<MeshAsset>(mr.Mesh);
        if (!mesh) continue;

        float distance;
        AABB box{
            glm::vec3(-0.5f),
            glm::vec3(0.5f)
        };
        if (RayIntersectsAABB(
            ray,
            box,
            transform.GetTransform(),
            distance))
        {
            if (distance < closestDist)
            {
                closestDist = distance;
                closest = entity;
            }
        }
    }

    return closest;
}

RenderLayer::RenderLayer(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    m_Renderer = std::make_unique<Renderer>();
}

void RenderLayer::OnAttach()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    std::memset(m_MouseDown, 0, sizeof(m_MouseDown));

    CreateFramebuffer(m_Width, m_Height);
    RenderTarget target;
    target.Framebuffer = m_Framebuffer;
    target.Width = m_Width;
    target.Height = m_Height;

    m_ViewportEvent = EventBus::Subscribe<ViewportEvent>(
        [this](const ViewportEvent& e)
        {
            m_MouseX = e.MouseX;
            m_MouseY = e.MouseY;
            m_ViewportX = e.ViewportX;
            m_ViewportY = e.ViewportY;
            m_ViewportHovered = e.Hovered;

            if (e.ViewportWidth == 0 || e.ViewportHeight == 0)
                return;

            if (m_ViewportWidth != e.ViewportWidth ||
                m_ViewportHeight != e.ViewportHeight)
            {
                // m_ViewportWidth = e.ViewportWidth;
                // m_ViewportHeight = e.ViewportHeight;

                // ResizeFramebuffer(m_ViewportWidth, m_ViewportHeight);
                // m_Camera.SetViewport(m_ViewportWidth, m_ViewportHeight);

                m_RequestedWidth = e.ViewportWidth;
                m_RequestedHeight = e.ViewportHeight;
                m_PendingResize = true;
            }
        }
    );

    m_MousePressEvent = EventBus::Subscribe<MouseButtonPressEvent>(
        [this](const MouseButtonPressEvent& e)
        {
            m_MouseDown[e.Button] = true;

            if (e.Button != GLFW_MOUSE_BUTTON_LEFT || !m_ViewportHovered)
                return;

            entt::entity picked = PickEntity(m_MouseX, m_MouseY);
            EditorContext::SetSelectedEntity(picked);

            if (picked != entt::null && m_GizmoMode != GizmoMode::None)
            {
                auto scene = Project::GetActive()->GetActiveScene();
                auto& registry = scene->GetRegistry();
                auto& tc = registry.get<TransformComponent>(picked);

                m_GizmoActive = true;
                m_GizmoStartMouse = { m_MouseX, m_MouseY };
                m_GizmoStartPosition = tc.Translation;
                m_GizmoStartRotation = tc.Rotation;
            }

        }
    );

    m_MouseReleaseEvent = EventBus::Subscribe<MouseButtonReleaseEvent>(
        [this](const MouseButtonReleaseEvent& e)
        {
            m_MouseDown[e.Button] = false;

            if (e.Button == GLFW_MOUSE_BUTTON_LEFT)
            {
                m_GizmoActive = false;
            }

        }
    );

    m_MouseMoveEvent = EventBus::Subscribe<MouseMoveEvent>(
        [this](const MouseMoveEvent& e)
        {
            if (!m_ViewportHovered)
                return;

            glm::vec2 delta(e.DeltaX, e.DeltaY);

            // bool alt =
                // m_KeyDown[GLFW_KEY_LEFT_ALT] ||
                // m_KeyDown[GLFW_KEY_RIGHT_ALT];

            bool alt = Input::IsKeyPressed(KeyCode::LeftAlt) ||
                Input::IsKeyPressed(KeyCode::RightAlt);

            if (alt)
            {
                if (m_MouseDown[GLFW_MOUSE_BUTTON_LEFT])
                {
                    m_Camera.Orbit(delta);
                }
                else if (m_MouseDown[GLFW_MOUSE_BUTTON_RIGHT])
                    m_Camera.Pan(delta);
            }
            else if (m_MouseDown[GLFW_MOUSE_BUTTON_RIGHT])
            {
                m_Camera.Rotate(delta.x, -delta.y);
            }
        }
    );

    m_MouseScrollEvent = EventBus::Subscribe<MouseScrollEvent>(
        [this](const MouseScrollEvent& e)
        {
            if (m_ViewportHovered)
                m_Camera.Zoom(e.Y);
        }
    );

    m_Renderer->Init(target);

    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE); 
}

void RenderLayer::OnDetach()
{
    glDeleteFramebuffers(1, &m_Framebuffer);
    glDeleteTextures(1, &m_ColorAttachment);
}

void RenderLayer::OnUpdate(float dt)
{
    if (m_PendingResize)
    {
        ResizeFramebuffer(m_RequestedWidth, m_RequestedHeight);
        m_Camera.SetViewport(m_RequestedWidth, m_RequestedHeight);

        m_ViewportWidth = m_RequestedWidth;
        m_ViewportHeight = m_RequestedHeight;

        m_PendingResize = false;
    }

    if (!m_ViewportHovered)
        return;

    if (m_GizmoActive)
    {
        // if (m_KeyDown[GLFW_KEY_X]) m_GizmoAxis = GizmoAxis::X;
        // if (m_KeyDown[GLFW_KEY_Y]) m_GizmoAxis = GizmoAxis::Y;
        // if (m_KeyDown[GLFW_KEY_Z]) m_GizmoAxis = GizmoAxis::Z;
        if (Input::IsKeyPressed(KeyCode::X)) m_GizmoAxis = GizmoAxis::X;
        if (Input::IsKeyPressed(KeyCode::Y)) m_GizmoAxis = GizmoAxis::Y;
        if (Input::IsKeyPressed(KeyCode::Z)) m_GizmoAxis = GizmoAxis::Z;
    }

    if (Input::IsKeyPressed(KeyCode::G))
    {
        m_GizmoMode = GizmoMode::Translate;
    }

    if (Input::IsKeyPressed(KeyCode::R))
    {
        m_GizmoMode = GizmoMode::Rotate;
    }

    // if (m_KeyDown[GLFW_KEY_ESCAPE])
    if (Input::IsKeyPressed(KeyCode::Escape))
    {
        m_GizmoMode = GizmoMode::None;
        m_GizmoAxis = GizmoAxis::Free;
        m_GizmoActive = false;
    }

    if (Input::IsKeyPressed(KeyCode::W)) m_Camera.Move(m_Camera.GetForward(), dt);
    // if (m_KeyDown[GLFW_KEY_S]) m_Camera.Move(-m_Camera.GetForward(), dt);
    // if (m_KeyDown[GLFW_KEY_A]) m_Camera.Move(-m_Camera.GetRight(), dt);
    // if (m_KeyDown[GLFW_KEY_D]) m_Camera.Move(m_Camera.GetRight(), dt);
    // if (m_KeyDown[GLFW_KEY_Q]) m_Camera.Move(-m_Camera.GetUp(), dt);
    // if (m_KeyDown[GLFW_KEY_E]) m_Camera.Move(m_Camera.GetUp(), dt);
    if (Input::IsKeyPressed(KeyCode::S)) m_Camera.Move(-m_Camera.GetForward(), dt);
    if (Input::IsKeyPressed(KeyCode::A)) m_Camera.Move(-m_Camera.GetRight(), dt);
    if (Input::IsKeyPressed(KeyCode::D)) m_Camera.Move(m_Camera.GetRight(), dt);
    if (Input::IsKeyPressed(KeyCode::Q)) m_Camera.Move(-m_Camera.GetUp(), dt);
    if (Input::IsKeyPressed(KeyCode::E)) m_Camera.Move(m_Camera.GetUp(), dt);

    if (m_GizmoActive && m_GizmoMode == GizmoMode::Translate)
    {
        entt::entity e = EditorContext::GetSelectedEntity();
        if (e == entt::null)
            return;

        auto scene = Project::GetActive()->GetActiveScene();
        auto& registry = scene->GetRegistry();
        auto& tc = registry.get<TransformComponent>(e);

        glm::vec2 mouseDelta =
            glm::vec2(m_MouseX, m_MouseY) - m_GizmoStartMouse;

        float sensitivity = 0.01f;

        glm::vec3 right = m_Camera.GetRight();
        glm::vec3 up = m_Camera.GetUp();
        glm::vec3 forward = m_Camera.GetForward();

        glm::vec3 offset =
            right * mouseDelta.x * sensitivity +
            up * -mouseDelta.y * sensitivity;

        // AXIS LOCK
        switch (m_GizmoAxis)
        {
        case GizmoAxis::X:
            offset = glm::vec3(offset.x, 0.0f, 0.0f);
            break;
        case GizmoAxis::Y:
            offset = glm::vec3(0.0f, offset.y, 0.0f);
            break;
        case GizmoAxis::Z:
        {
            glm::vec3 axis = glm::vec3(0, 0, 1);

            // Use camera right projected onto plane perpendicular to Z
            glm::vec3 camRight = m_Camera.GetRight();
            camRight -= axis * glm::dot(camRight, axis);
            camRight = glm::normalize(camRight);

            float movement = mouseDelta.x * sensitivity;
            offset = axis * movement;
            break;
        }
        default:
            break;
        }

        tc.Translation = m_GizmoStartPosition + offset;
    }


    if (m_GizmoActive && m_GizmoMode == GizmoMode::Rotate)
    {
        entt::entity e = EditorContext::GetSelectedEntity();
        if (e == entt::null)
            return;

        auto scene = Project::GetActive()->GetActiveScene();
        auto& registry = scene->GetRegistry();
        auto& tc = registry.get<TransformComponent>(e);

        float deltaX = m_MouseX - m_GizmoStartMouse.x;
        float sensitivity = 0.005f;

        glm::vec3 axis;

        switch (m_GizmoAxis)
        {
        case GizmoAxis::X:
            axis = glm::normalize(m_GizmoStartRotation * glm::vec3(1, 0, 0));
            break;
        case GizmoAxis::Y:
            axis = glm::normalize(m_GizmoStartRotation * glm::vec3(0, 1, 0));
            break;
        case GizmoAxis::Z:
            axis = glm::normalize(m_GizmoStartRotation * glm::vec3(0, 0, 1));
            break;
        default:
            axis = glm::normalize(m_Camera.GetForward());
            break;
        }

        float angle = deltaX * sensitivity;
        glm::quat delta = glm::angleAxis(angle, axis);

        tc.Rotation = delta * m_GizmoStartRotation;
    }
}

void RenderLayer::OnRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, m_Width, m_Height);
    glClearColor((float)80 / 256, (float)80 / 256, (float)80 / 256, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    auto project = Project::GetActive();
    if (!project)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    auto scene = project->GetActiveScene();
    if (!scene)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // static bool s_Spawned = false;
    // if (!s_Spawned)
    // {
    //     auto assetMan = project->GetAssetManager();
    //     auto cube = assetMan->ImportAsset("../donut2.glb");
    //     scene->CreateMeshEntity("Cube", cube, assetMan->GetDefaultMaterial());
    //     // auto plane = assetMan->ImportAsset("plane.obj");
    //     // scene->CreateMeshEntity("plane", plane, assetMan->GetDefaultMaterial());
    //     // scene->CreateLightEntity("light 1");
    //     s_Spawned = true;
    // }

    FrameData frame;
    frame.View = m_Camera.GetView();
    frame.Projection = m_Camera.GetProjection();
    frame.CameraPosition = m_Camera.GetPosition();

    m_Renderer->BeginFrame(frame);
    m_Renderer->RenderScene(*scene);
    m_Renderer->EndFrame();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_Width, m_Height);

    EventBus::Publish(NewFrameRenderedEvent(
        m_ColorAttachment,
        m_Width,
        m_Height
    ));
}

void RenderLayer::CreateFramebuffer(uint32_t width, uint32_t height)
{
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    // Color
    glGenTextures(1, &m_ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        m_ColorAttachment,
        0
    );

    // Depth
    glGenTextures(1, &m_DepthAttachment);
    glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
        width, height, 0,
        GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr
    );

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_TEXTURE_2D,
        m_DepthAttachment,
        0
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Editor framebuffer incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderLayer::ResizeFramebuffer(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;

    // Delete old resources
    if (m_Framebuffer)
    {
        glDeleteFramebuffers(1, &m_Framebuffer);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteTextures(1, &m_DepthAttachment);
    }

    CreateFramebuffer(width, height);

    RenderTarget target;
    target.Framebuffer = m_Framebuffer;
    target.Width = width;
    target.Height = height;

    m_Renderer->OnResize(target);
}
