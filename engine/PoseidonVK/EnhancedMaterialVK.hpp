#pragma once
#include <string_view>
#include <algorithm>
#include <cctype>
#include <string>
#include <array>
#include <cstdint>

namespace Poseidon::vk
{
// Small explicit opt-in catalog. Identity includes the model, visual LOD and
// section, not just a shared texture name. Parameters are conservative estimates,
// not measured optics. Empty map paths independently retain stock inputs.
struct EnhancedMaterial
{
    unsigned id;
    const char *model, *section, *albedo, *normal, *roughnessMap;
    float roughness, metallic;
    int width, height;
    uint64_t textureFingerprint;
};
inline constexpr EnhancedMaterial StoneWall{
    1, "data3d\\zed_kamenna.p3d", "data\\kamennazed.pac", "", "", "", .9f, 0.f, 128, 128, 3735404598107056867ull};
inline constexpr EnhancedMaterial ConcreteCap{
    2, "data3d\\zed2.p3d", "data\\plot_beton_03_top.pac", "", "", "", .9f, 0.f, 256, 32, 18240797119409424594ull};
inline constexpr EnhancedMaterial ConcretePanel{
    3, "data3d\\zed2.p3d", "data\\plot_beton_0302_side.pac", "", "", "", .9f, 0.f, 256, 128, 16860270304147451178ull};
inline constexpr EnhancedMaterial ConcretePillar{
    4, "data3d\\zed2.p3d", "data\\plot_beton_02_side.pac", "", "", "", .9f, 0.f, 256, 128, 6362336912512726729ull};
inline constexpr std::array EnhancedCatalog{&StoneWall, &ConcreteCap, &ConcretePanel, &ConcretePillar};

inline uint64_t EnhancedGeometryFingerprint(unsigned id, float lod)
{
    if (id == StoneWall.id)
        return lod == .5f ? 2195626591502288280ull : lod == 1.f ? 15759270531097937650ull : 0;
    if (id >= ConcreteCap.id && id <= ConcretePillar.id)
        return lod == .5f   ? 8442782668387980414ull
               : lod == 1.f ? 12645244065637725224ull
               : lod == 4.f ? 13954822052462775290ull
                            : 0;
    return 0;
}

inline bool EnhancedSourceCompatible(const EnhancedMaterial& material, float lod, uint64_t geometry, int width,
                                     int height, uint64_t pixels)
{
    const auto expected = EnhancedGeometryFingerprint(material.id, lod);
    return expected && geometry == expected && width == material.width && height == material.height &&
           pixels == material.textureFingerprint;
}

inline std::string MaterialIdentity(std::string_view path)
{
    std::string key(path);
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c) { return c == '/' ? '\\' : char(std::tolower(c)); });
    return key;
}
inline const EnhancedMaterial* FindEnhancedMaterial(std::string_view model, float lod, std::string_view section,
                                                    int vertices, int sections)
{
    // Inspected stock visual LODs only; memory/collision/shadow and substituted
    // LODs do not inherit the enhancement. Modified assets require explicit opt-in.
    const auto key = MaterialIdentity(model), texture = MaterialIdentity(section);
    for (const auto* entry : EnhancedCatalog)
    {
        const auto& material = *entry;
        if (key != material.model || texture != material.section)
            continue;
        const bool stone = material.id == StoneWall.id && sections == 1 &&
                           ((lod == .5f && vertices == 48) || (lod == 1.f && vertices == 24));
        const bool concrete = material.id != StoneWall.id &&
                              ((sections == 3 && ((lod == .5f && vertices == 68) || (lod == 1.f && vertices == 20))) ||
                               (sections == 2 && lod == 4.f && vertices == 8 && material.id != ConcreteCap.id));
        if (stone || concrete)
            return entry;
    }
    return nullptr;
}
} // namespace Poseidon::vk
