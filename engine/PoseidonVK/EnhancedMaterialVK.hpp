#pragma once
#include <string_view>
#include <algorithm>
#include <cctype>
#include <string>

namespace Poseidon::vk
{
// Small explicit opt-in catalog. Identity includes the model, visual LOD and
// section, not just a shared texture name. Parameters are authored, not measured.
struct EnhancedMaterial
{
    unsigned id;
    const char *model, *section, *albedo, *normalRoughness;
    float roughness, metallic;
};
inline constexpr EnhancedMaterial StoneWall{1,
                                            "data3d\\zed_kamenna.p3d",
                                            "data\\kamennazed.pac",
                                            "materials/enhanced/stone-wall/albedo.paa",
                                            "materials/enhanced/stone-wall/normal-roughness.paa",
                                            .9f,
                                            0.f};

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
    if (MaterialIdentity(model) != StoneWall.model || section != StoneWall.section || sections != 1)
        return nullptr;
    // Inspected stock visual LODs only; memory/collision/shadow and substituted
    // LODs do not inherit the enhancement. Modified assets require explicit opt-in.
    if ((lod == .5f && vertices == 48) || (lod == 1.f && vertices == 24))
        return &StoneWall;
    return nullptr;
}
} // namespace Poseidon::vk
