#include "InspectorPanel.h"

#include "imgui.h"
#include "EditorContext.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

InspectorPanel::InspectorPanel(SceneController* sceneController)
    : m_SceneController(sceneController)
{
}

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

    auto& tag = registry.get<TagComponent>(entity);
    char buffer[256] = {};
    strncpy(buffer, tag.Tag.c_str(), sizeof(buffer) - 1);

    if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        tag.Tag = buffer;

    ImGui::SameLine();
    ImGui::TextDisabled("(Entity)");

    ImGui::Separator();

    DrawComponent<TransformComponent>("Transform", scene, entity);
    DrawComponent<RigidBodyComponent>("Rigid Body", scene, entity);
    DrawComponent<BoxColliderComponent>("Box Collider", scene, entity);
    DrawComponent<SphereColliderComponent>("Sphere Collider", scene, entity);
    DrawComponent<DistanceJointComponent>("Distance Joint", scene, entity);

    ImGui::Separator();

    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponentPopup");

    DrawAddComponentPopup(scene, entity);
}

template<typename T>
void InspectorPanel::DrawComponent(const char* name,
    std::shared_ptr<Scene> scene,
    entt::entity entity)
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
                component.Rotation = glm::quat(glm::radians(euler));

            ImGui::DragFloat3("Scale", &component.Scale.x, 0.1f, 0.1f);
        }

        if constexpr (std::is_same_v<T, RigidBodyComponent>)
        {
            ImGui::DragFloat("Mass", &component.Mass, 0.1f);
            ImGui::DragFloat("Restitution", &component.Restitution, 0.01f);
            ImGui::DragFloat("Friction", &component.Friction, 0.01f);
            ImGui::Checkbox("Is static", &component.IsStatic);
        }

        if constexpr (std::is_same_v<T, BoxColliderComponent>)
        {
            ImGui::DragFloat3("Half Extents", &component.HalfExtents.x, 0.1f);
        }

        if constexpr (std::is_same_v<T, SphereColliderComponent>)
        {
            ImGui::DragFloat("Radius", &component.Radius, 0.1f);
        }

        if constexpr (std::is_same_v<T, DistanceJointComponent>)
        {
            auto& comp = component;

            DrawEntityPicker(scene, comp.ConnectedEntity, "Connected Entity");

            ImGui::DragFloat3("Local Anchor A", &comp.LocalAnchorA.x, 0.1f);
            ImGui::DragFloat3("Local Anchor B", &comp.LocalAnchorB.x, 0.1f);
            ImGui::Text("Target Length: %f", comp.TargetLength);

            if (registry.valid(comp.ConnectedEntity))
            {
                auto& trA = registry.get<TransformComponent>(entity);
                auto& trB = registry.get<TransformComponent>(comp.ConnectedEntity);

                glm::vec3 worldA = trA.Translation + trA.Rotation * comp.LocalAnchorA;
                glm::vec3 worldB = trB.Translation + trB.Rotation * comp.LocalAnchorB;

                comp.TargetLength = glm::length(worldA - worldB);
            }
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
        if (!registry.any_of<RigidBodyComponent>(entity))
        {
            if (ImGui::MenuItem("Rigid Body"))
                registry.emplace<RigidBodyComponent>(entity);
        }

        if (!registry.any_of<BoxColliderComponent>(entity))
        {
            if (ImGui::MenuItem("Box Collider"))
                registry.emplace<BoxColliderComponent>(entity);
        }

        if (!registry.any_of<SphereColliderComponent>(entity))
        {
            if (ImGui::MenuItem("Sphere Collider"))
                registry.emplace<SphereColliderComponent>(entity);
        }

        if (!registry.any_of<DistanceJointComponent>(entity))
        {
            if (ImGui::MenuItem("Distance Joint"))
            {
                m_PendingJointEntity = entity;
                m_CreateDistanceJoint = true;
                m_OpenJointTargetPopup = true;
                ImGui::OpenPopup("SelectJointTarget");
            }
        }

        ImGui::EndPopup();
    }

    if (m_OpenJointTargetPopup)
    {
        ImGui::OpenPopup("SelectJointTarget");
        m_OpenJointTargetPopup = false;
    }

    // Joint target selection popup
    if (ImGui::BeginPopup("SelectJointTarget"))
    {
        registry.view<TagComponent, RigidBodyComponent>().each(
            [&](auto candidate, auto& tag, auto&)
            {
                if (candidate == m_PendingJointEntity)
                    return;

                if (ImGui::Selectable(tag.Tag.c_str()))
                {
                    if (m_CreateDistanceJoint)
                    {
                        m_SceneController->CreateDistanceJoint(
                            m_PendingJointEntity,
                            candidate,
                            glm::vec3(0.0f),
                            glm::vec3(0.0f)
                        );
                    }

                    m_CreateDistanceJoint = false;
                    m_PendingJointEntity = entt::null;

                    ImGui::CloseCurrentPopup();
                }
            });

        ImGui::EndPopup();
    }
}

void InspectorPanel::DrawEntityPicker(std::shared_ptr<Scene> scene,
    entt::entity& target,
    const char* label)
{
    auto& registry = scene->GetRegistry();

    const char* preview = "None";

    if (target != entt::null && registry.valid(target))
        preview = registry.get<TagComponent>(target).Tag.c_str();

    if (ImGui::BeginCombo(label, preview))
    {
        registry.view<TagComponent, RigidBodyComponent>().each(
            [&](auto candidate, auto& tag, auto&)
            {
                if (candidate == EditorContext::GetSelectedEntity())
                    return;

                bool selected = (candidate == target);

                if (ImGui::Selectable(tag.Tag.c_str(), selected))
                    target = candidate;

                if (selected)
                    ImGui::SetItemDefaultFocus();
            });

        ImGui::EndCombo();
    }
}