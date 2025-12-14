#include "EditorSettings.h"

#include <nlohmann/json.hpp>
#include <fstream>

static std::filesystem::path GetSettingsPath()
{
    return std::filesystem::path(std::getenv("APPDATA")) / "Physim/EditorSettings.json";
}

EditorSettings EditorSettings::Load()
{
    EditorSettings settings;
    auto path = GetSettingsPath();

    if (!std::filesystem::exists(path))
        return settings;

    std::ifstream in(path);
    nlohmann::json j;
    in >> j;

    for (auto& p : j["RecentProjects"])
        settings.RecentProjects.emplace_back(p.get<std::string>());

    settings.LastProject = j.value("LastProject", "");

    return settings;
}

void EditorSettings::Save() const
{
    auto path = GetSettingsPath();
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json j;
    j["LastProject"] = LastProject.string();
    for (auto& p : RecentProjects)
        j["RecentProjects"].push_back(p.string());

    std::ofstream out(path);
    out << j.dump(4);
}

void EditorSettings::AddRecentProject(const std::filesystem::path& path)
{
    RecentProjects.erase(
        std::remove(RecentProjects.begin(), RecentProjects.end(), path),
        RecentProjects.end()
    );

    RecentProjects.insert(RecentProjects.begin(), path);

    if (RecentProjects.size() > 10)
        RecentProjects.resize(10);

    LastProject = path;
}
