#include "Renderer.h"

#include <entt/entity/registry.hpp>
#include "project/Project.h"
#include <iostream>
#include "Camera.h"
#include "EditorContext.h"

inline std::shared_ptr<Mesh> CreateAxisMeshX(float length)
{
    std::vector<Vertex> v =
    {
        { { -length , 0, 0 }, {}, {} },
        { { length, 0, 0 }, {}, {} }
    };

    return std::make_shared<Mesh>(v, std::vector<uint32_t>{0, 1});
}

inline std::shared_ptr<Mesh> CreateAxisMeshZ(float length)
{
    std::vector<Vertex> v =
    {
        { { 0, 0, -length }, {}, {} },
        { { 0, 0, length }, {}, {} }
    };

    return std::make_shared<Mesh>(v, std::vector<uint32_t>{0, 1});
}

static std::shared_ptr<Mesh> CreateWireCube()
{
    std::vector<Vertex> v =
    {
        {{-1,-1,-1},{},{}}, {{1,-1,-1},{},{}},
        {{1,1,-1},{},{}},   {{-1,1,-1},{},{}},
        {{-1,-1,1},{},{}},  {{1,-1,1},{},{}},
        {{1,1,1},{},{}},    {{-1,1,1},{},{}}
    };

    std::vector<uint32_t> i =
    {
        0,1, 1,2, 2,3, 3,0, // back
        4,5, 5,6, 6,7, 7,4, // front
        0,4, 1,5, 2,6, 3,7  // connections
    };

    return std::make_shared<Mesh>(v, i);
}

static std::shared_ptr<Mesh> CreateWireSphere(int segments = 32)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t index = 0;

    // Create multiple latitude circles
    int latitudes = 8;
    for (int lat = 0; lat < latitudes; lat++)
    {
        float phi = (float)lat / (latitudes - 1) * glm::pi<float>();

        for (int i = 0; i <= segments; i++)
        {
            float theta = (float)i / segments * glm::two_pi<float>();

            glm::vec3 p = {
                sin(phi) * cos(theta),
                cos(phi),
                sin(phi) * sin(theta)
            };

            vertices.push_back({ p, {}, {} });

            if (i > 0)
            {
                indices.push_back(index - 1);
                indices.push_back(index);
            }

            index++;
        }
    }

    // Create longitude lines
    for (int lon = 0; lon < segments; lon++)
    {
        float theta = (float)lon / segments * glm::two_pi<float>();

        for (int i = 0; i <= segments; i++)
        {
            float phi = (float)i / segments * glm::pi<float>();

            glm::vec3 p = {
                sin(phi) * cos(theta),
                cos(phi),
                sin(phi) * sin(theta)
            };

            vertices.push_back({ p, {}, {} });

            if (i > 0)
            {
                indices.push_back(index - 1);
                indices.push_back(index);
            }

            index++;
        }
    }

    return std::make_shared<Mesh>(vertices, indices);
}


void Renderer::Init(const RenderTarget& target)
{
    glCreateFramebuffers(1, &m_ShadowFBO);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_ShadowMap);

    glTextureStorage2D(
        m_ShadowMap,
        1,
        GL_DEPTH_COMPONENT32F,
        SHADOW_SIZE,
        SHADOW_SIZE
    );

    glTextureParameteri(m_ShadowMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_ShadowMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_ShadowMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_ShadowMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTextureParameterfv(m_ShadowMap, GL_TEXTURE_BORDER_COLOR, border);

    glNamedFramebufferTexture(
        m_ShadowFBO,
        GL_DEPTH_ATTACHMENT,
        m_ShadowMap,
        0
    );

    glNamedFramebufferDrawBuffer(m_ShadowFBO, GL_NONE);
    glNamedFramebufferReadBuffer(m_ShadowFBO, GL_NONE);

    m_Target = target;

    m_ShadowShader = std::make_shared<Shader>("../engine/shaders/depth.vert", "../engine/shaders/depth.frag");
    m_ForwardShader = std::make_shared<Shader>("../engine/shaders/forward.vert", "../engine/shaders/forward.frag");
    m_OutlineShader = std::make_shared<Shader>("../engine/shaders/outline.vert", "../engine/shaders/outline.frag");

    m_GizmoShader = std::make_shared<Shader>("../engine/shaders/gizmo.vert", "../engine/shaders/gizmo.frag");

    m_GridShader = std::make_shared<Shader>("../engine/shaders/grid.vert", "../engine/shaders/grid.frag");
    m_GridMesh = CreateGrid(100, 1.0f);
    m_AxisX = CreateAxisMeshX(100.0f);
    m_AxisZ = CreateAxisMeshZ(100.0f);

    m_DebugCube = CreateWireCube();
    m_DebugSphere = CreateWireSphere();

}

void Renderer::BeginFrame(const FrameData& frame)
{
    m_Frame = frame;

    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glBindFramebuffer(GL_FRAMEBUFFER, m_Target.Framebuffer);
    // glViewport(0, 0, m_Target.Width, m_Target.Height);
}

void Renderer::EndFrame()
{
    // Present handled by window system
}

void Renderer::OnResize(const RenderTarget& target)
{
    m_Target = target;
}

void Renderer::RenderScene(Scene& scene)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_Target.Framebuffer);
    glViewport(0, 0, m_Target.Width, m_Target.Height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glDisable(GL_STENCIL_TEST);
    RenderGrid();

    // FORWARD PASS
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    CollectLights(scene);
    ForwardPass(scene);
    // RenderColliders(scene);

    // EditorPass(scene);

    // OUTLINE PASS
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    OutlinePass(scene);

    RenderColliders(scene);

    // FULL RESET
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glDisable(GL_STENCIL_TEST);
}

void Renderer::ShadowPass(Scene& scene)
{
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_ShadowShader->Bind();

    auto& registry = scene.GetRegistry();
    auto view = registry.view<TransformComponent, MeshRenderComponent>();

    for (auto [entity, transform, mr] : view.each())
    {
        auto mesh = AssetManager::GetAsset<MeshAsset>(mr.Mesh);
        if (!mesh) continue;

        m_ShadowShader->SetMat4f("u_Model", transform.GetTransform());
        mesh->MeshData->Draw();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_Target.Framebuffer);
}

void Renderer::ForwardPass(Scene& scene)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_Target.Framebuffer);
    glViewport(0, 0, m_Target.Width, m_Target.Height);

    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_STENCIL_TEST);

    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_ForwardShader->Bind();
    m_ForwardShader->SetMat4f("u_View", m_Frame.View);
    m_ForwardShader->SetMat4f("u_Projection", m_Frame.Projection);
    m_ForwardShader->SetVec3f("u_ViewPos", m_Frame.CameraPosition);

    UploadLights(m_ForwardShader);

    entt::entity selected = EditorContext::GetSelectedEntity();

    auto view = scene.GetRegistry()
        .view<TransformComponent, MeshRenderComponent>();

    for (auto [entity, transform, mr] : view.each())
    {
        auto mesh = AssetManager::GetAsset<MeshAsset>(mr.Mesh);
        auto material = AssetManager::GetAsset<MaterialAsset>(mr.Material);
        if (!mesh || !material) continue;

        // Mark selected object in stencil
        if (entity == selected)
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
        else
            glStencilFunc(GL_ALWAYS, 0, 0xFF);

        m_ForwardShader->SetMat4f("u_Model", transform.GetTransform());
        material->MaterialData->Bind(*m_ForwardShader);
        mesh->MeshData->Draw();
    }
}

void Renderer::OutlinePass(Scene& scene)
{
    entt::entity selected = EditorContext::GetSelectedEntity();
    if (selected == entt::null)
        return;

    auto& registry = scene.GetRegistry();
    if (!registry.all_of<TransformComponent, MeshRenderComponent>(selected))
        return;

    auto& transform = registry.get<TransformComponent>(selected);
    auto& mr = registry.get<MeshRenderComponent>(selected);
    auto mesh = AssetManager::GetAsset<MeshAsset>(mr.Mesh);
    if (!mesh) return;

    glDisable(GL_DEPTH_TEST);

    m_OutlineShader->Bind();
    m_OutlineShader->SetMat4f("u_View", m_Frame.View);
    m_OutlineShader->SetMat4f("u_Projection", m_Frame.Projection);
    m_OutlineShader->SetVec3f("u_OutlineColor", { 1.0f, 0.58f, 0.0f });

    const float scale = 1.05f;

    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), transform.Translation) *
        glm::mat4_cast(transform.Rotation) *
        glm::scale(glm::mat4(1.0f), transform.Scale * scale);

    m_OutlineShader->SetMat4f("u_Model", model);
    mesh->MeshData->Draw();

    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::EditorPass(Scene& scene)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC1_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // optional

    m_GizmoShader->Bind();
    m_GizmoShader->SetMat4f("u_View", m_Frame.View);
    m_GizmoShader->SetMat4f("u_Projection", m_Frame.Projection);

    auto& registry = scene.GetRegistry();
    auto view = registry.view<TransformComponent, LightComponent>();

    for (auto [entity, transform, light] : view.each())
    {
        glm::mat4 model(1.0f);

        switch (light.Type)
        {
        case LightType::Point:
        {
            model = MakeBillboard(transform.Translation, m_Frame.View, 0.25f);

            m_GizmoShader->SetVec3f("u_Color", light.Color);
            m_GizmoShader->SetMat4f("u_Model", model);
            m_QuadMesh->Draw();
            break;
        }

        case LightType::Spot:
        {
            glm::vec3 dir = transform.Rotation * glm::vec3(0, 0, -1);

            model =
                glm::translate(glm::mat4(1.0f), transform.Translation) *
                glm::mat4_cast(transform.Rotation) *
                glm::scale(glm::mat4(1.0f),
                    glm::vec3(light.Range));

            m_GizmoShader->SetVec3f("u_Color", light.Color);
            m_GizmoShader->SetMat4f("u_Model", model);
            m_ConeMesh->Draw();
            break;
        }

        case LightType::Directional:
        {
            model =
                glm::translate(glm::mat4(1.0f), transform.Translation) *
                glm::mat4_cast(transform.Rotation);

            m_GizmoShader->SetVec3f("u_Color", light.Color);
            m_GizmoShader->SetMat4f("u_Model", model);
            m_DirectionalMesh->Draw();
            break;
        }
        }
    }

    RenderColliders(scene);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::UploadLights(const std::shared_ptr<Shader>& shader)
{
    shader->Set1i("u_LightCount", (int)m_Lights.size());

    for (size_t i = 0; i < m_Lights.size(); i++)
    {
        const auto& light = m_Lights[i];
        std::string base = "u_Lights[" + std::to_string(i) + "]";

        shader->Set3f(base + ".Position", light.Position.x, light.Position.y, light.Position.z);
        shader->Set3f(base + ".Color", light.Color.x, light.Color.y, light.Color.z);
        shader->Set1f(base + ".Intensity", light.Intensity);
        shader->Set1i(base + ".Type", static_cast<int>(light.Type));
        shader->Set3f(base + ".Direction", light.Direction.x, light.Direction.y, light.Direction.z);
        shader->Set1f(base + ".Range", light.Range);
    }
}

void Renderer::CollectLights(Scene& scene)
{
    m_Lights.clear();

    auto& registry = scene.GetRegistry();
    auto view = registry.view<TransformComponent, LightComponent>();

    for (auto [entity, transform, light] : view.each())
    {
        RendererLight rl{};
        rl.Position = transform.Translation; // or GetPosition()
        rl.Color = light.Color;
        rl.Intensity = light.Intensity;
        rl.Type = light.Type;
        rl.Range = light.Range;

        // Directional lights use forward vector
        if (light.Type == LightType::Directional)
        {
            rl.Direction = glm::normalize(
                transform.Rotation * glm::vec3(0.0f, 0.0f, -1.0f)
            );
        }
        else
        {
            rl.Direction = glm::vec3(0.0f);
        }

        m_Lights.push_back(rl);

        // Optional: cap max lights
        if (m_Lights.size() >= 32)
            break;
    }
}

glm::mat4 Renderer::MakeBillboard(
    const glm::vec3& position,
    const glm::mat4& view,
    float size
)
{
    glm::mat3 cameraRotation = glm::mat3(glm::inverse(view));

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(cameraRotation[0] * size, 0.0f);
    model[1] = glm::vec4(cameraRotation[1] * size, 0.0f);
    model[2] = glm::vec4(cameraRotation[2] * size, 0.0f);
    model[3] = glm::vec4(position, 1.0f);

    return model;
}

std::shared_ptr<Mesh> Renderer::CreateGrid(
    int halfSize,
    float spacing
)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t index = 0;

    for (int i = -halfSize; i <= halfSize; i++)
    {
        float p = i * spacing;

        // Z lines
        vertices.push_back({ { -halfSize * spacing, 0.0f, p }, {}, { } });
        vertices.push_back({ {  halfSize * spacing, 0.0f, p }, {}, { } });
        indices.push_back(index++);
        indices.push_back(index++);

        // X lines
        vertices.push_back({ { p, 0.0f, -halfSize * spacing }, {}, { } });
        vertices.push_back({ { p, 0.0f,  halfSize * spacing }, {}, { } });
        indices.push_back(index++);
        indices.push_back(index++);
    }

    return std::make_shared<Mesh>(vertices, indices);
}

void Renderer::RenderGrid()
{
    // Grid must NOT touch depth or stencil
    // glDisable(GL_DEPTH_TEST);
    // glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_GridShader->Bind();
    m_GridShader->SetMat4f("u_View", m_Frame.View);
    m_GridShader->SetMat4f("u_Projection", m_Frame.Projection);
    m_GridShader->SetVec3f("u_CameraPos", m_Frame.CameraPosition);

    m_GridShader->SetVec3f("u_Color", { 1.0f, 0.0f, 0.0f });
    m_AxisX->DrawLines();

    m_GridShader->SetVec3f("u_Color", { 0.0f, 0.0f, 1.0f });
    m_AxisZ->DrawLines();

    m_GridShader->SetVec3f("u_Color", { 0.4f, 0.4f, 0.4f });
    m_GridMesh->DrawLines();

    // Restore depth for rest of pipeline
    // glDepthMask(GL_TRUE);
    // glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::RenderColliders(Scene& scene)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_STENCIL_TEST);

    m_GizmoShader->Bind();
    m_GizmoShader->SetMat4f("u_View", m_Frame.View);
    m_GizmoShader->SetMat4f("u_Projection", m_Frame.Projection);

    auto& registry = scene.GetRegistry();

    // ----------------------------
    // Box Colliders
    // ----------------------------
    {
        auto view = registry.view<TransformComponent, BoxColliderComponent>();

        for (auto [entity, transform, box] : view.each())
        {
            glm::mat4 model =
                glm::translate(glm::mat4(1.0f), transform.Translation) *
                glm::mat4_cast(transform.Rotation) *
                glm::scale(glm::mat4(1.0f),
                    box.HalfExtents * 2.0f);

            m_GizmoShader->SetVec3f("u_Color", { 0.0f, 1.0f, 0.0f });
            m_GizmoShader->SetMat4f("u_Model", model);

            m_DebugCube->DrawLines();
        }
    }

    // ----------------------------
    // Sphere Colliders
    // ----------------------------
    {
        auto view = registry.view<TransformComponent, SphereColliderComponent>();

        for (auto [entity, transform, sphere] : view.each())
        {
            glm::mat4 model =
                glm::translate(glm::mat4(1.0f), transform.Translation) *
                glm::mat4_cast(transform.Rotation) *
                glm::scale(glm::mat4(1.0f),
                    glm::vec3(sphere.Radius));

            m_GizmoShader->SetVec3f("u_Color", { 0.2f, 0.8f, 1.0f });
            m_GizmoShader->SetMat4f("u_Model", model);

            m_DebugSphere->DrawLines();
        }
    }

    glDisable(GL_BLEND);
}
