#pragma once

#include "Asset.h"

#include <filesystem>

struct AssetMetadata
{
    AssetType Type;
    std::filesystem::path FilePath;

    std::filesystem::path VertexPath;
    std::filesystem::path FragmentPath;

    operator bool() const { return Type != AssetType::None; }
};