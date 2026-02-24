#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Model.h"
#include "LightType.h"
#include "scene/Scene.h"
#include "asset/Asset.h"
#include "RenderTarget.h"

struct RendererLight
{
    glm::vec3 Position;
    float Intensity;

    glm::vec3 Color;
    LightType Type; // 0 = directional, 1 = point, 2 = spot

    glm::vec3 Direction;
    float Range;
};

class Renderer
{
public:
    void Init(const RenderTarget& target);
    void BeginFrame(const FrameData& frame);
    void RenderScene(Scene& scene);
    void EndFrame();

    void OnResize(const RenderTarget& target);
private:
    void ShadowPass(Scene& scene);
    void ForwardPass(Scene& scene);
    void OutlinePass(Scene& scene);
    void EditorPass(Scene& scene);

    void CollectLights(Scene& scene);
    void UploadLights(const std::shared_ptr<Shader>& shader);
    glm::mat4 MakeBillboard(const glm::vec3& position, const glm::mat4& view, float size);
    std::shared_ptr<Mesh> CreateGrid(int halfSize, float spacing);
    std::shared_ptr<Mesh> CreateAxisMesh(float length);
    void RenderGrid();
    void RenderColliders(Scene& scene);
    void RenderDistanceJoints(Scene& scene);

private:
    FrameData m_Frame;
    RenderTarget m_Target;

    // shadow
    uint32_t m_ShadowFBO = 0;
    uint32_t m_ShadowMap = 0;
    static constexpr uint32_t SHADOW_SIZE = 2048;

    // that's all the shaders I need in this case
    // therefore there's no reason for me to make them inherit from an Asset class
    // all materials use the same shader with different uniforms 
    std::shared_ptr<Shader> m_ShadowShader;
    std::shared_ptr<Shader> m_OutlineShader;
    std::shared_ptr<Shader> m_ForwardShader;

    std::vector<RendererLight> m_Lights;

    std::shared_ptr<Mesh> m_QuadMesh;
    std::shared_ptr<Mesh> m_ConeMesh;
    std::shared_ptr<Mesh> m_DirectionalMesh; // arrow or line

    std::shared_ptr<Mesh> m_GridMesh;
    std::shared_ptr<Mesh> m_AxisX;
    std::shared_ptr<Mesh> m_AxisZ;

    std::shared_ptr<Shader> m_GizmoShader;
    std::shared_ptr<Shader> m_GridShader;

    std::shared_ptr<Mesh> m_DebugCube;
    std::shared_ptr<Mesh> m_DebugSphere;

    std::shared_ptr<Mesh> m_DebugLine;
    std::shared_ptr<Mesh> m_DebugAnchorSphere;
};
