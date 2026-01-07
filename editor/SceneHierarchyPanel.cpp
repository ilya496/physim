#include "SceneHierarchyPanel.h"

#include "EditorContext.h"

SceneHierarchyPanel::SceneHierarchyPanel()
{
    m_MeshIcon = Texture::Create("../editor/icons/mesh-icon.png", false);
    m_LightIcon = Texture::Create("../editor/icons/light-icon.png", false);
    // m_EmptyIcon = Texture::Create("../editor/icons/entity-icon.png");
}

void SceneHierarchyPanel::Draw(std::shared_ptr<Scene> scene)
{
    ImGui::Begin("Scene Hierarchy");

    ImGuiTreeNodeFlags rootFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_FramePadding;

    bool rootOpen = ImGui::TreeNodeEx("Scene", rootFlags);

    if (rootOpen)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<IDComponent>();

        for (auto entityHandle : view)
        {
            Entity entity{ entityHandle, scene.get() };
            DrawEntityNode(entity);
        }

        ImGui::TreePop();
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        EditorContext::SetSelectedEntity({});

    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Add Empty Entity"))
            scene->CreateEntity("Empty Entity");

        if (ImGui::BeginMenu("Add Light"))
        {
            if (ImGui::MenuItem("Point Light"))
            {
                scene->CreateLightEntity("Light");
            }

            if (ImGui::MenuItem("Spot Light", nullptr, false, false))
            {
            }

            if (ImGui::MenuItem("Directional Light", nullptr, false, false))
            {
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity)
{
    const std::string& name = entity.GetName();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_FramePadding |
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (EditorContext::GetSelectedEntity() == entity)
        flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(entity.GetHandle());

    bool opened = ImGui::TreeNodeEx(
        "##Entity",
        flags,
        "%s",
        name.empty() ? "Unnamed Entity" : name.c_str()
    );

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        EditorContext::SetSelectedEntity(entity);

    ImGui::PopID();
}

ImTextureID SceneHierarchyPanel::GetEntityIcon(Entity entity)
{
    if (entity.HasComponent<MeshRenderComponent>())
        return (ImTextureID)m_MeshIcon->GetRendererID();

    if (entity.HasComponent<LightComponent>())
        return (ImTextureID)m_LightIcon->GetRendererID();

    return (ImTextureID)m_EmptyIcon->GetRendererID();
}
