#include "AssetPanel.h"

#include "project/Project.h"
#include "asset/AssetManager.h"
#include "utils/FileDialog.h"

AssetPanel::AssetPanel()
{
    m_MeshIcon = std::make_shared<Texture>("../editor/icons/mesh-icon.png");
    m_TextureIcon = std::make_shared<Texture>("../editor/icons/texture-icon.png");
    m_MaterialIcon = std::make_shared<Texture>("../editor/icons/material-icon.png");
}

void AssetPanel::Draw(std::shared_ptr<Scene> scene)
{
    ImGui::Begin("Assets");

    std::shared_ptr<AssetManager> assetManager = Project::GetActive()->GetAssetManager();
    auto registry = assetManager->GetAssetRegistry();

    static char searchBuffer[128] = {};
    ImGui::PushItemWidth(-120.0f);
    ImGui::InputTextWithHint("##AssetSearch", "Search assets...", searchBuffer, sizeof(searchBuffer));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Import"))
    {
        auto path = FileDialog::OpenFile("Select asset", "*");
        assetManager->ImportAsset(path);
    }

    ImGui::Separator();

    ImGui::BeginChild("AssetTabs", ImVec2(170, 0), true);

    if (ImGui::Selectable("Meshes", m_CurrentTab == AssetTab::Meshes))
        m_CurrentTab = AssetTab::Meshes;

    if (ImGui::Selectable("Textures", m_CurrentTab == AssetTab::Textures))
        m_CurrentTab = AssetTab::Textures;

    if (ImGui::Selectable("Materials", m_CurrentTab == AssetTab::Materials))
        m_CurrentTab = AssetTab::Materials;

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("AssetGrid", ImVec2(0, 0), false);

    static float thumbnailSize = 72.0f;
    static float padding = 16.0f;

    float cellSize = thumbnailSize + padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::Columns(columnCount, nullptr, false);

    for (auto& [handle, metadata] : registry)
    {
        if (!AssetMatchesTab(metadata.Type, m_CurrentTab))
            continue;

        const std::string name = metadata.FilePath.stem().string();

        if (searchBuffer[0] != 0 &&
            name.find(searchBuffer) == std::string::npos)
            continue;

        ImGui::PushID((int)handle);

        ImTextureID icon = GetIconForAssetType(metadata.Type);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::ImageButton("##AssetIcon", icon,
            { thumbnailSize, thumbnailSize }, { 0,1 }, { 1,0 });
        ImGui::PopStyleColor();

        if (ImGui::IsItemClicked())
        {
            // TODO: EditorContext::SelectAsset(handle);
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload(
                "CONTENT_BROWSER_ITEM",
                &handle,
                sizeof(AssetHandle)
            );
            ImGui::Text("%s", name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginPopupContextItem())
        {
            ImGui::Text("%s", metadata.FilePath.filename().string().c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Show in Explorer"))
            {
                // platform open
            }

            if (ImGui::MenuItem("Reload"))
            {
                // assetManager->ReloadAsset(handle);
            }

            if (ImGui::MenuItem("Delete"))
            {
                // assetManager->RemoveAsset(handle);
            }

            ImGui::EndPopup();
        }

        ImGui::TextWrapped("%s", name.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 32.0f, 128.0f);

    ImGui::End();
}

ImTextureID AssetPanel::GetIconForAssetType(AssetType type)
{
    switch (type)
    {
    case AssetType::Mesh:     return (ImTextureID)m_MeshIcon->GetRendererID();
    case AssetType::Texture:  return (ImTextureID)m_TextureIcon->GetRendererID();
    case AssetType::Material: return (ImTextureID)m_MaterialIcon->GetRendererID();
    default:                  return 0;
    }
}

bool AssetPanel::AssetMatchesTab(AssetType type, AssetTab tab)
{
    switch (tab)
    {
    case AssetTab::Meshes:     return type == AssetType::Mesh;
    case AssetTab::Textures:   return type == AssetType::Texture;
    case AssetTab::Materials:  return type == AssetType::Material;
    default:                   return false;
    }
}
