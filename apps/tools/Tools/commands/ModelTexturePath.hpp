#pragma once
#include <algorithm>
#include <filesystem>
#include <string>

namespace PoseidonTools
{
// Qualification needs the exact reference, never the preview's basename search.
inline std::filesystem::path QualificationTexturePath(const std::filesystem::path& model,
    const std::filesystem::path& textureRoot, std::string texture)
{
    std::replace(texture.begin(), texture.end(), '\\', '/');
    return (textureRoot.empty() ? model.parent_path() : textureRoot) / texture;
}
}
