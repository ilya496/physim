#include "SceneSerializer.h"

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "Components.h"
#include "Entity.h"
#include "render/LightType.h"
#include <iostream>

namespace nlohmann
{
    template<>
    struct adl_serializer<glm::vec3>
    {
        static void to_json(json& j, const glm::vec3& v)
        {
            j = json{ v.x, v.y, v.z };
        }

        static void from_json(const json& j, glm::vec3& v)
        {
            v.x = j.at(0).get<float>();
            v.y = j.at(1).get<float>();
            v.z = j.at(2).get<float>();
        }
    };

    template<>
    struct adl_serializer<glm::quat>
    {
        static void to_json(json& j, const glm::quat& q)
        {
            j = json{ q.w, q.x, q.y, q.z };
        }

        static void from_json(const json& j, glm::quat& q)
        {
            q.w = j.at(0).get<float>();
            q.x = j.at(1).get<float>();
            q.y = j.at(2).get<float>();
            q.z = j.at(3).get<float>();
        }
    };

    template<>
    struct adl_serializer<UUID>
    {
        static void to_json(json& j, const UUID& uuid)
        {
            j = uuid.string();
        }

        static void from_json(const json& j, UUID& uuid)
        {
            uuid = UUID(j.get<std::string>());
        }
    };
}

inline std::string LightTypeToString(LightType type)
{
    switch (type)
    {
    case LightType::None: return "None";
    case LightType::Point: return "Point";
    case LightType::Directional: return "Directional";
    case LightType::Spot: return "Spot";
    default:
        return "<Invalid>";
    }
}

inline LightType LightTypeFromString(const std::string& type)
{
    if (type == "None") return LightType::None;
    if (type == "Point") return LightType::Point;
    if (type == "Directional") return LightType::Directional;
    if (type == "Spot") return LightType::Spot;

    return LightType::None;
}

SceneSerializer::SceneSerializer(const std::shared_ptr<Scene>& scene)
    : m_Scene(scene)
{
}

bool SceneSerializer::Serialize(const std::filesystem::path& filepath)
{
    json root;
    root["Scene"] = "Untitled";

    json entities = json::array();

    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<IDComponent>();

    for (auto entityID : view)
    {
        Entity entity{ entityID, m_Scene.get() };
        json e;

        e["Entity"] = entity.GetUUID();

        if (entity.HasComponent<TagComponent>())
        {
            auto& tc = entity.GetComponent<TagComponent>();
            e["TagComponent"]["Tag"] = tc.Tag;
        }

        if (entity.HasComponent<TransformComponent>())
        {
            auto& tc = entity.GetComponent<TransformComponent>();
            e["TransformComponent"] = {
                { "Translation", tc.Translation },
                { "Rotation", tc.Rotation },
                { "Scale", tc.Scale }
            };
        }

        if (entity.HasComponent<MeshRenderComponent>())
        {
            auto& mc = entity.GetComponent<MeshRenderComponent>();
            e["MeshRenderComponent"] = {
                { "Mesh", mc.Mesh },
                { "Material", mc.Material }
            };
        }

        if (entity.HasComponent<RigidBodyComponent>())
        {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            e["RigidBodyComponent"] = {
                { "Mass", rb.Mass },
                { "IsStatic", rb.IsStatic },
                // { "Velocity", rb.Velocity },
                // { "AngularVelocity", rb.AngularVelocity },
                { "Restitution", rb.Restitution },
                { "Friction", rb.Friction }
            };
        }

        if (entity.HasComponent<BoxColliderComponent>())
        {
            auto& bc = entity.GetComponent<BoxColliderComponent>();
            e["BoxColliderComponent"]["HalfExtents"] = bc.HalfExtents;
        }

        if (entity.HasComponent<SphereColliderComponent>())
        {
            auto& sc = entity.GetComponent<SphereColliderComponent>();
            e["SphereColliderComponent"]["Radius"] = sc.Radius;
        }

        if (entity.HasComponent<LightComponent>())
        {
            auto& lc = entity.GetComponent<LightComponent>();
            e["LightComponent"] = {
                { "Type", LightTypeToString(lc.Type) },
                { "Color", lc.Color },
                { "Intensity", lc.Intensity },
                { "Range", lc.Range },
                { "Direction", lc.Direction }
            };
        }

        entities.push_back(e);
    }

    root["Entities"] = entities;

    std::ofstream out(filepath);
    out << root.dump(4);
    return true;
}

bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
{
    std::ifstream in(filepath);
    if (!in.is_open())
        return false;

    json root;
    in >> root;

    auto entities = root["Entities"];

    for (auto& e : entities)
    {
        UUID uuid = e["Entity"].get<UUID>();
        std::string name = "Entity";

        if (e.contains("TagComponent"))
            name = e["TagComponent"]["Tag"];

        Entity entity = m_Scene->CreateEntityWithUUID(uuid, name);

        if (e.contains("TransformComponent"))
        {
            auto& tc = entity.AddComponent<TransformComponent>();
            tc.Translation = e["TransformComponent"]["Translation"];
            tc.Rotation = e["TransformComponent"]["Rotation"];
            tc.Scale = e["TransformComponent"]["Scale"];
        }

        if (e.contains("MeshRenderComponent"))
        {
            auto& mc = entity.AddComponent<MeshRenderComponent>();
            mc.Mesh = e["MeshRenderComponent"]["Mesh"];
            mc.Material = e["MeshRenderComponent"]["Material"];
        }

        if (e.contains("RigidBodyComponent"))
        {
            auto& rb = entity.AddComponent<RigidBodyComponent>();
            rb.Mass = e["RigidBodyComponent"]["Mass"];
            rb.IsStatic = e["RigidBodyComponent"]["IsStatic"];
            // rb.Velocity = e["RigidBodyComponent"]["Velocity"];
            // rb.AngularVelocity = e["RigidBodyComponent"]["AngularVelocity"];
            rb.Restitution = e["RigidBodyComponent"]["Restitution"];
            rb.Friction = e["RigidBodyComponent"]["Friction"];
        }

        if (e.contains("BoxColliderComponent"))
        {
            auto& bc = entity.AddComponent<BoxColliderComponent>();
            bc.HalfExtents = e["BoxColliderComponent"]["HalfExtents"];
        }

        if (e.contains("SphereColliderComponent"))
        {
            auto& sc = entity.AddComponent<SphereColliderComponent>();
            sc.Radius = e["SphereColliderComponent"]["Radius"];
        }

        if (e.contains("LightComponent"))
        {
            std::cout << "contains light\n";
            auto& lc = entity.AddComponent<LightComponent>();
            lc.Type = LightTypeFromString(e["LightComponent"]["Type"]);
            lc.Color = e["LightComponent"]["Color"];
            lc.Intensity = e["LightComponent"]["Intensity"];
            lc.Range = e["LightComponent"]["Range"];
            lc.Direction = e["LightComponent"]["Direction"];
        }
    }

    return true;
}
