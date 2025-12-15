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

    if (j.contains("RecentProjects"))
    {
        for (auto& p : j["RecentProjects"])
        {
            auto str = p.get<std::string>();
            if (!str.empty())
                settings.RecentProjects.emplace_back(str);
        }
    }

    if (j.contains("LastProject"))
    {
        auto str = j["LastProject"].get<std::string>();
        if (!str.empty())
            settings.LastProject = str;
    }

    return settings;
}

void EditorSettings::Save() const
{
    auto path = GetSettingsPath();
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json j;

    if (!LastProject.empty())
        j["LastProject"] = LastProject.string();

    j["RecentProjects"] = nlohmann::json::array();
    for (const auto& p : RecentProjects)
    {
        if (!p.empty())
            j["RecentProjects"].push_back(p.string());
    }

    std::ofstream out(path);
    out << j.dump(4);
}

void EditorSettings::AddRecentProject(const std::filesystem::path& path)
{
    if (path.empty())
        return;

    std::filesystem::path normalized = std::filesystem::weakly_canonical(path);

    // remove empty entries and duplicates
    for (auto it = RecentProjects.begin(); it != RecentProjects.end(); )
    {
        if (it->empty())
        {
            it = RecentProjects.erase(it);
            continue;
        }

        if (std::filesystem::equivalent(*it, normalized))
        {
            it = RecentProjects.erase(it);
            continue;
        }

        ++it;
    }

    RecentProjects.insert(RecentProjects.begin(), normalized);

    while (RecentProjects.size() > 10)
        RecentProjects.pop_back();

    LastProject = normalized;
}

