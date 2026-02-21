#include "SceneHierarchyPanel.h"

#include "EditorContext.h"

void SceneHierarchyPanel::Draw(std::shared_ptr<Scene> scene)
{
    ImGui::Begin("Scene Hierarchy");

    Entity entityToDelete{};

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

            if (DrawEntityNode(entity))
                entityToDelete = entity;
        }

        ImGui::TreePop();
    }

    if (entityToDelete)
    {
        if (EditorContext::GetSelectedEntity() == entityToDelete)
            EditorContext::SetSelectedEntity({});

        scene->DestroyEntity(entityToDelete);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        EditorContext::SetSelectedEntity({});

    if (ImGui::BeginPopupContextWindow(
        "SceneHierarchyWindowContext",
        ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems))
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

bool SceneHierarchyPanel::DrawEntityNode(Entity entity)
{
    bool entityDeleted = false;

    const std::string& name = entity.GetName();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_FramePadding |
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (EditorContext::GetSelectedEntity() == entity)
        flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(entity.GetHandle());

    ImGui::TreeNodeEx(
        "##Entity",
        flags,
        "%s",
        name.empty() ? "Unnamed Entity" : name.c_str()
    );

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        EditorContext::SetSelectedEntity(entity);

    if (EditorContext::GetSelectedEntity() == entity &&
        ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        entityDeleted = true;
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete"))
            entityDeleted = true;

        ImGui::EndPopup();
    }

    ImGui::PopID();

    return entityDeleted;
}