#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>
#include <iterator>

namespace Poseidon::vk
{
struct StockPBRMaterial
{
    std::string_view texture;
    float roughness;
    unsigned receiverFamily; // 0 legacy, 1 catalog, 2 rock, 3 roof, 4 wood, 5 facade, 6 stone, 7 concrete
};
// The unchanged inspected v3 allowlist. No prefix/brightness heuristics or
// promotion of mixed window/metal atlases. Identities are shared by raster/hits.
inline constexpr StockPBRMaterial StockPBRMaterials[]{
    {"data\\skala_piskovec2.pac", .85f, 2},      {"data\\strecha_tasky_vln.pac", .90f, 3},
    {"data\\strecha_tasky_vln90.pac", .90f, 3},  {"data\\prkna_dark.pac", .85f, 4},
    {"data\\telegraf_sloup.pac", .85f, 4},       {"data\\prkna_dalsi.pac", .85f, 4},
    {"data\\tasky_tmavsi.pac", .90f, 3},         {"data\\kamennazed.pac", .90f, 6},
    {"data\\omlacena_bocni_zed.pac", .90f, 5},   {"data\\stodola_f.pac", .90f, 5},
    {"data\\stodola_f2.pac", .90f, 5},           {"data\\stodola_s.pac", .90f, 5},
    {"data\\betondlouhastenabez.pac", .90f, 7},  {"data\\betonpoprask_side.pac", .90f, 7},
    {"data\\betonpoprask_top.pac", .90f, 7},     {"data\\plot_beton_01_side.pac", .90f, 7},
    {"data\\plot_beton_02_side.pac", .90f, 7},   {"data\\plot_beton_03_side.pac", .90f, 7},
    {"data\\plot_beton_0301_side.pac", .90f, 7}, {"data\\plot_beton_0302_side.pac", .90f, 7},
    {"data\\plot_beton_01_top.pac", .90f, 7},    {"data\\plot_beton_03_top.pac", .90f, 7},
    // Painted signpost remains PBR/reflector-only; do not adopt uncertain cutout coverage.
    {"data\\sloupek_bily_orezly_.pac", 0, 0}};
inline unsigned StockPBRIdentity(std::string_view texture)
{
    for (unsigned i = 0; i < std::size(StockPBRMaterials); ++i)
        if (texture == StockPBRMaterials[i].texture)
            return 64 + i;
    return 0;
}
// Additional inspected rough dielectric sections are enhanced-mode only. Keep
// the v3 allowlist/identities unchanged. Reuse the original receiver families;
// classification alone never bypasses the captured-ambient/opaque-world gates.
inline constexpr StockPBRMaterial EnhancedPBRMaterials[]{
    {"data\\tasky_p90.pac", .90f, 3}, {"data\\budova2_stit2.pac", .90f, 5}};
inline unsigned PBRReceiverFamily(unsigned id)
{
    if (id >= 64 && id < 64 + std::size(StockPBRMaterials)) return StockPBRMaterials[id - 64].receiverFamily;
    if (id >= 128 && id < 128 + std::size(EnhancedPBRMaterials)) return EnhancedPBRMaterials[id - 128].receiverFamily;
    return 0;
}
inline unsigned EnhancedPBRIdentity(std::string_view texture)
{
    if (const auto id = StockPBRIdentity(texture)) return id;
    for (unsigned i = 0; i < std::size(EnhancedPBRMaterials); ++i)
        if (texture == EnhancedPBRMaterials[i].texture) return 128 + i;
    return 0;
}
inline float PBRMaterial(std::string_view texture, int legacyPower, bool enhanced = false)
{
    const auto id = StockPBRIdentity(texture);
    if (!id)
    {
        if (enhanced)
            if (const auto extra = EnhancedPBRIdentity(texture); extra >= 128)
                return EnhancedPBRMaterials[extra - 128].roughness;
        return 0;
    }
    if (const float roughness = StockPBRMaterials[id - 64].roughness; roughness > 0)
        return roughness;
    // Existing painted-signpost lobe-width hint, not a measured conversion.
    return legacyPower > 0 ? std::clamp(std::pow(2.f / (float(legacyPower) + 2.f), .25f), .5f, .65f) : .6f;
}
} // namespace Poseidon::vk
