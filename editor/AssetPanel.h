#pragma once

#include <memory>
#include "scene/Scene.h"
#include "render/Model.h"
#include "imgui.h"

enum class AssetTab
{
    Meshes,
    Textures,
    Materials
};

class AssetPanel
{
public:
    AssetPanel();

    void Draw(std::shared_ptr<Scene> scene);

private:
    ImTextureID GetIconForAssetType(AssetType type);
    bool AssetMatchesTab(AssetType type, AssetTab tab);

    std::shared_ptr<Texture> m_MeshIcon;
    std::shared_ptr<Texture> m_TextureIcon;
    std::shared_ptr<Texture> m_MaterialIcon;

    AssetTab m_CurrentTab = AssetTab::Meshes;
};