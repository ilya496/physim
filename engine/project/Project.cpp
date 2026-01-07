#include "Project.h"

#include "asset/AssetManager.h"
#include "ProjectSerializer.h"
#include "scene/SceneSerializer.h"
#include <iostream>

std::filesystem::path Project::GetAssetAbsolutePath(const std::filesystem::path& path)
{
    return GetAssetDirectory() / path;
}

std::shared_ptr<Project> Project::New()
{
    s_ActiveProject = std::make_shared<Project>();
    return s_ActiveProject;
}

std::shared_ptr<Project> Project::Load(const std::filesystem::path& path)
{
    std::shared_ptr<Project> project = std::make_shared<Project>();

    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>();
        SceneSerializer sceneSerializer(scene);

        if (sceneSerializer.Deserialize(path.parent_path() / "assets" / "main.scene"))
        {
            project->m_ProjectDirectory = path.parent_path();
            s_ActiveProject = project;

            std::shared_ptr<AssetManager> assetManager = std::make_shared<AssetManager>();

            s_ActiveProject->m_AssetManager = assetManager;
            s_ActiveProject->m_ActiveScene = scene;

            assetManager->DeserializeAssetRegistry();

            return s_ActiveProject;
        }
    }

    return nullptr;
}

bool Project::SaveActive(const std::filesystem::path& path)
{
    s_ActiveProject->m_Config.Name = path.filename().string();
    ProjectSerializer serializer(s_ActiveProject);
    if (serializer.Serialize(path))
    {
        s_ActiveProject->m_ProjectDirectory = path.parent_path();
        SceneSerializer sceneSerializer(s_ActiveProject->m_ActiveScene);

        if (sceneSerializer.Serialize(path.parent_path() / "assets" / "main.scene"))
        {
            return true;
        }

    }

    return false;
}

void Project::Close()
{
    if (s_ActiveProject)
    {
        if (s_ActiveProject->m_AssetManager)
        {
            s_ActiveProject->m_AssetManager.reset();
        }
        s_ActiveProject.reset();
    }
}