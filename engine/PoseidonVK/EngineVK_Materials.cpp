#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <PoseidonVK/EnhancedMaterialMapsVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <filesystem>
#include <fstream>
#include <SDL3/SDL_filesystem.h>

namespace Poseidon
{
bool EngineVK::ApplyEnhancedMaterial(const Shape& shape, const TextureVK& texture, vk::ShapeLighting& lighting,
                                     std::shared_ptr<vk::TextureImage>& albedo,
                                     std::shared_ptr<vk::TextureImage>& normal)
{
    if (!_vk.EnhancedMaterialsEnabled() || texture.IsDynamic())
        return false;
    const auto model = vk::MaterialIdentity(static_cast<const char*>(shape.SourceModel()));
    if (!std::any_of(vk::EnhancedCatalog.begin(), vk::EnhancedCatalog.end(),
                     [&](const auto* m) { return model == m->model && texture.MaterialKey() == m->section; }))
        return false;
    const auto [source, unseen] =
        _enhancedSourceTextures.try_emplace(std::make_pair(&texture, shape.SourceResolution()), 0);
    const auto& pixels = texture.Pixels();
    if (unseen)
    {
        uint64_t hash = 1469598103934665603ull;
        for (auto byte : pixels.rgba)
            hash = (hash ^ byte) * 1099511628211ull;
        source->second = hash;
        LOG_INFO(
            Graphics, "Enhanced source {} {}x{} fingerprint={} model={} lod={} vertices={} sections={} geometry={}",
            texture.MaterialKey(), pixels.width, pixels.height, hash, static_cast<const char*>(shape.SourceModel()),
            shape.SourceResolution(), shape.NVertex(), shape.NSections(), shape.SourceFingerprint());
    }
    const auto* material = vk::FindEnhancedMaterial(model, shape.SourceResolution(), texture.MaterialKey(),
                                                    shape.NVertex(), shape.NSections());
    if (!material)
        return false;
    if (!vk::EnhancedSourceCompatible(*material, shape.SourceResolution(), shape.SourceFingerprint(), pixels.width,
                                      pixels.height, source->second))
        return false;
    auto [entry, first] = _enhancedMaps.try_emplace(material->id);
    auto& maps = entry->second;
    if (first)
    {
        const auto root = std::filesystem::path(SDL_GetBasePath());
        // Missing optional inputs fail independently. Stock color, geometry
        // normals and scalar roughness remain a complete enhanced material.
        if (*material->albedo)
            try
            {
                maps.albedo = _textures.Load((root / material->albedo).string().c_str());
            }
            catch (const std::exception& e)
            {
                LOG_WARN(Graphics, "Enhanced albedo fallback: {}", e.what());
            }
        auto load = [&](const char* name)
        {
            std::vector<DecodedImage> levels;
            if (!*name)
                return levels;
            try
            {
                const auto path = root / name;
                std::ifstream file(path, std::ios::binary);
                std::vector<char> bytes((std::istreambuf_iterator<char>(file)), {});
                levels = DecodePAAMipChainBuffer(bytes.data(), bytes.size(), path.extension() == ".paa");
                if (levels.empty())
                    throw std::runtime_error("invalid supplemental data map");
            }
            catch (const std::exception& e)
            {
                LOG_WARN(Graphics, "Enhanced map {} fallback: {}", name, e.what());
            }
            return levels;
        };
        const auto normals = load(material->normal), roughness = load(material->roughnessMap);
        maps.normal = !normals.empty();
        maps.roughness = !roughness.empty();
        const auto packed = vk::PackEnhancedMaps(normals, roughness);
        if (!packed.empty())
        {
            std::vector<vk::TextureMip> mips;
            for (const auto& mip : packed)
                mips.push_back({uint32_t(mip.width), uint32_t(mip.height), mip.rgba.data()});
            maps.data = _vk.UploadTexture(mips);
            maps.dataPixels = packed.front();
        }
        LOG_INFO(Graphics, "Enhanced stock material {} ready; override={} normal={} roughness={}", material->id,
                 bool(maps.albedo), maps.normal, maps.roughness);
    }
    auto* color = dynamic_cast<TextureVK*>(maps.albedo.GetRef());
    if (!color)
        color = const_cast<TextureVK*>(&texture);
    albedo = color->Image(_vk);
    normal = maps.data; // Empty means original normals and scalar roughness.
    // Raster and hit evaluation use identical decoded base levels. Retained by
    // the existing TextureImage/frame/BLAS ownership and fence lifetimes.
    if (_vk.RTHitDataEnabled())
    {
        const auto& p = color->Pixels();
        _vk.PrepareRTAOAlpha(albedo, p.width, p.height, p.rgba.data());
        if (maps.data)
            _vk.PrepareRTAOAlpha(maps.data, maps.dataPixels.width, maps.dataPixels.height, maps.dataPixels.rgba.data());
    }
    lighting.pbr = {material->roughness, .04f, material->metallic, float(material->id)};
    auto rgba = [](ColorVal c, bool map) { return std::array<float, 4>{c.R(), c.G(), c.B(), float(map)}; };
    lighting.materialAmbient = rgba(GScene->MainLight()->Ambient() * GetAccomodateEye(), maps.normal);
    lighting.materialSun = rgba(GScene->MainLight()->Diffuse() * GetAccomodateEye(), maps.roughness);
    return true;
}
} // namespace Poseidon
