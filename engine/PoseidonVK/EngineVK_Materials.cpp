#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <filesystem>
#include <map>
#include <SDL3/SDL_filesystem.h>

namespace Poseidon
{
bool EngineVK::ApplyEnhancedMaterial(const Shape& shape, const TextureVK& texture, vk::ShapeLighting& lighting,
                                     std::shared_ptr<vk::TextureImage>& albedo,
                                     std::shared_ptr<vk::TextureImage>& normal)
{
    if (!_vk.EnhancedMaterialsEnabled())
        return false;
    const auto* material =
        vk::FindEnhancedMaterial(static_cast<const char*>(shape.SourceModel()), shape.SourceResolution(),
                                 texture.MaterialKey(), shape.NVertex(), shape.NSections());
    if (!material)
        return false;
    const auto [source, unseen] = _enhancedSourceTextures.try_emplace(&texture, false);
    if (unseen)
    {
        const auto& pixels = texture.Pixels();
        uint64_t hash = 1469598103934665603ull;
        for (auto byte : pixels.rgba)
            hash = (hash ^ byte) * 1099511628211ull;
        LOG_INFO(Graphics, "Enhanced source texture {}x{} fingerprint={}", pixels.width, pixels.height, hash);
        source->second = pixels.width == 128 && pixels.height == 128 && hash == 3735404598107056867ull;
    }
    if (!source->second)
        return false;
    const auto expected = shape.SourceResolution() == .5f ? 2195626591502288280ull : 15759270531097937650ull;
    if (shape.SourceFingerprint() != expected)
        return false;
    if (!_enhancedLoadAttempted)
    {
        _enhancedLoadAttempted = true;
        try
        {
            const auto root = std::filesystem::path(SDL_GetBasePath());
            _enhancedAlbedo = _textures.Load((root / material->albedo).string().c_str());
            _enhancedNormal = _textures.Load((root / material->normalRoughness).string().c_str());
            if (!_enhancedAlbedo || !_enhancedNormal)
                throw std::runtime_error("missing supplemental maps");
            LOG_INFO(Graphics, "Enhanced material stone-wall-v1 loaded (authored albedo, normal/roughness)");
        }
        catch (const std::exception& error)
        {
            _enhancedAlbedo = nullptr;
            _enhancedNormal = nullptr;
            LOG_WARN(Graphics, "Enhanced material unavailable; keeping stock PBR: {}", error.what());
        }
    }
    auto* color = dynamic_cast<TextureVK*>(_enhancedAlbedo.GetRef());
    auto* data = dynamic_cast<TextureVK*>(_enhancedNormal.GetRef());
    if (!color || !data)
        return false;
    albedo = color->Image(_vk);
    normal = data->Image(_vk);
    // Raster and hit evaluation use identical decoded base levels. Retained by
    // the existing TextureImage/frame/BLAS ownership and fence lifetimes.
    if (_vk.RTHitDataEnabled())
    {
        for (auto* map : {color, data})
        {
            const auto& p = map->Pixels();
            _vk.PrepareRTAOAlpha(map->Image(_vk), p.width, p.height, p.rgba.data());
        }
    }
    lighting.pbr = {material->roughness, .04f, material->metallic, float(material->id)};
    auto rgba = [](ColorVal c) { return std::array<float, 4>{c.R(), c.G(), c.B(), 0}; };
    lighting.materialAmbient = rgba(GScene->MainLight()->Ambient() * GetAccomodateEye());
    lighting.materialSun = rgba(GScene->MainLight()->Diffuse() * GetAccomodateEye());
    return true;
}
} // namespace Poseidon
