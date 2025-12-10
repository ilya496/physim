#include "AssetImporter.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::shared_ptr<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
{
    switch (metadata.Type)
    {
    case AssetType::Texture: return ImportTexture(handle, metadata);
    case AssetType::Scene: return ImportScene(handle, metadata);
    default:
        return nullptr;
    }
}

std::shared_ptr<Texture> AssetImporter::ImportTexture(AssetHandle handle, const AssetMetadata& metadata)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = nullptr;
    std::string path = metadata.FilePath.string();
    data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    // TextureSpecification spec;
    // spec.Width = width;
    // spec.Height = height;
    switch (channels)
    {
    case 3:
        // spec.Format = ImageFormat::RGB8;
        break;
    case 4:
        // spec.Format = ImageFormat::RGBA8;
        break;
    }
    return std::make_shared<Texture>();
}

std::shared_ptr<Scene> AssetImporter::ImportScene(AssetHandle handle, const AssetMetadata& metadata)
{
    return nullptr;
}