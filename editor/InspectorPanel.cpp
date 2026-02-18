#include "InspectorPanel.h"

#include "imgui.h"
#include "EditorContext.h"

void InspectorPanel::Draw(std::shared_ptr<Scene> scene)
{
    ImGui::Begin("Inspector");

    entt::entity entity = EditorContext::GetSelectedEntity();
    if (entity == entt::null)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    DrawEntityInspector(scene, entity);

    ImGui::End();
}

void InspectorPanel::DrawEntityInspector(std::shared_ptr<Scene> scene, entt::entity entity)
{
    auto& registry = scene->GetRegistry();

    // name
    auto& tag = registry.get<TagComponent>(entity);
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, tag.Tag.c_str(), sizeof(buffer) - 1);

    if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        tag.Tag = buffer;

    ImGui::SameLine();
    ImGui::TextDisabled("(Entity)");

    ImGui::Separator();

    // components
    DrawComponent<TransformComponent>("Transform", scene, entity);
    DrawComponent<MeshRenderComponent>("Mesh", scene, entity);
    DrawComponent<RigidBodyComponent>("Rigid Body", scene, entity);
    DrawComponent<BoxColliderComponent>("Box Collider", scene, entity);
    DrawComponent<SphereColliderComponent>("Sphere Collider", scene, entity);

    ImGui::Separator();

    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponentPopup");

    DrawAddComponentPopup(scene, entity);
}

template<typename T>
void InspectorPanel::DrawComponent(const char* name, std::shared_ptr<Scene> scene, entt::entity entity)
{
    auto& registry = scene->GetRegistry();
    if (!registry.all_of<T>(entity))
        return;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_AllowOverlap;

    bool open = ImGui::TreeNodeEx(
        (void*)typeid(T).hash_code(),
        flags,
        "%s",
        name
    );

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 24.0f);
    if (ImGui::Button(":"))
        ImGui::OpenPopup("ComponentSettings");

    bool removeComponent = false;
    if (ImGui::BeginPopup("ComponentSettings"))
    {
        if (ImGui::MenuItem("Remove component"))
            removeComponent = true;

        ImGui::EndPopup();
    }

    if (open)
    {
        auto& component = registry.get<T>(entity);

        if constexpr (std::is_same_v<T, TransformComponent>)
        {
            ImGui::DragFloat3("Translation", &component.Translation.x, 0.1f);
            glm::vec3 euler = glm::degrees(glm::eulerAngles(component.Rotation));

            if (ImGui::DragFloat3("Rotation", &euler.x, 0.1f))
            {
                component.Rotation = glm::quat(glm::radians(euler));
            }
            ImGui::DragFloat3("Scale", &component.Scale.x, 0.1f, 0.1f);
        }

        // if constexpr (std::is_same_v<T, MeshRenderComponent>)
        // {
        //     ImGui::Text("Mesh Handle: %llu", component.Mesh);
        //     ImGui::Text("Material Handle: %llu", component.Material);
        // }

        if constexpr (std::is_same_v<T, RigidBodyComponent>)
        {
            ImGui::DragFloat("Mass", &component.Mass, 0.1f);
            ImGui::DragFloat("Restitution", &component.Restitution, 0.01f);
            ImGui::DragFloat("Friction", &component.Friction, 0.01f);
            ImGui::Checkbox("Is static", &component.IsStatic);
        }

        if constexpr (std::is_same_v<T, BoxColliderComponent>)
        {
            ImGui::Text("Box Collider");
            ImGui::DragFloat3("Half Extents", &component.HalfExtents.x, 1.0f);
        }

        if constexpr (std::is_same_v<T, SphereColliderComponent>)
        {
            ImGui::Text("Sphere Collider");
            ImGui::DragFloat("Radius", &component.Radius, 1.0f);
        }

        ImGui::TreePop();
    }

    if (removeComponent)
        registry.remove<T>(entity);
}

void InspectorPanel::DrawAddComponentPopup(std::shared_ptr<Scene> scene, entt::entity entity)
{
    auto& registry = scene->GetRegistry();

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (!registry.any_of<TransformComponent>(entity))
        {
            if (ImGui::MenuItem("Transform"))
                registry.emplace<TransformComponent>(entity);
        }

        if (!registry.any_of<MeshRenderComponent>(entity))
        {
            if (ImGui::MenuItem("Mesh"))
                registry.emplace<MeshRenderComponent>(entity);
        }

        if (!registry.any_of<RigidBodyComponent>(entity))
        {
            if (ImGui::MenuItem("Rigid body"))
                registry.emplace<RigidBodyComponent>(entity);
        }

        if (!registry.any_of<BoxColliderComponent>(entity))
        {
            if (ImGui::MenuItem("Box collider"))
            {
                // Remove any existing collider first
                if (registry.any_of<SphereColliderComponent>(entity))
                    registry.remove<SphereColliderComponent>(entity);

                registry.emplace<BoxColliderComponent>(entity);
            }
        }

        if (!registry.any_of<SphereColliderComponent>(entity))
        {
            if (ImGui::MenuItem("Sphere collider"))
            {
                // Remove any existing collider first
                if (registry.any_of<BoxColliderComponent>(entity))
                    registry.remove<BoxColliderComponent>(entity);

                registry.emplace<SphereColliderComponent>(entity);
            }
        }


        ImGui::EndPopup();
    }
}