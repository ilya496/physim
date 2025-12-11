#include "ProjectSerializer.h"

#include <fstream>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

ProjectSerializer::ProjectSerializer(std::shared_ptr<Project> project)
    : m_Project(project)
{
}

bool ProjectSerializer::Serialize(const std::filesystem::path& filePath)
{
    if (!m_Project)
        return false;

    const ProjectConfig& config = m_Project->GetConfig();

    json root;

    root["Project"] = {
        { "Name", config.Name },
        { "StartScene", config.StartScene.string() },
        { "AssetDirectory", config.AssetDirectory.generic_string() },
        { "AssetRegistryPath", config.AssetRegistryPath.generic_string() }
    };

    std::ofstream out(filePath);
    if (!out.is_open())
        return false;

    out << root.dump(4);
    return true;
}


bool ProjectSerializer::Deserialize(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath))
        return false;

    std::ifstream in(filePath);
    if (!in.is_open())
        return false;

    nlohmann::json root;
    in >> root;

    if (!root.contains("Project"))
        return false;

    auto& pj = root["Project"];
    ProjectConfig& config = m_Project->GetConfig();

    config.Name = pj.at("Name").get<std::string>();

    std::string startSceneStr = pj.at("StartScene").get<std::string>();
    config.StartScene = AssetHandle(startSceneStr);

    config.AssetDirectory = pj.at("AssetDirectory").get<std::string>();
    config.AssetRegistryPath = pj.at("AssetRegistryPath").get<std::string>();

    return true;
}
