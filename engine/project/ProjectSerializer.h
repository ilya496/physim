#pragma once

#include "Project.h"

class ProjectSerializer
{
public:
    ProjectSerializer(std::shared_ptr<Project> project);

    bool Serialize(const std::filesystem::path& path);
    bool Deserialize(const std::filesystem::path& path);

private:
    std::shared_ptr<Project> m_Project;
};