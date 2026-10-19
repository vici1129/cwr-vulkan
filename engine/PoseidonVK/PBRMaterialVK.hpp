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
};
// The unchanged inspected v3 allowlist. No prefix/brightness heuristics or
// promotion of mixed window/metal atlases. Identities are shared by raster/hits.
inline constexpr StockPBRMaterial StockPBRMaterials[]{
    {"data\\skala_piskovec2.pac", .85f},      {"data\\strecha_tasky_vln.pac", .90f},
    {"data\\strecha_tasky_vln90.pac", .90f},  {"data\\prkna_dark.pac", .85f},
    {"data\\telegraf_sloup.pac", .85f},       {"data\\prkna_dalsi.pac", .85f},
    {"data\\tasky_tmavsi.pac", .90f},         {"data\\kamennazed.pac", .90f},
    {"data\\omlacena_bocni_zed.pac", .90f},   {"data\\stodola_f.pac", .90f},
    {"data\\stodola_f2.pac", .90f},           {"data\\stodola_s.pac", .90f},
    {"data\\betondlouhastenabez.pac", .90f},  {"data\\betonpoprask_side.pac", .90f},
    {"data\\betonpoprask_top.pac", .90f},     {"data\\plot_beton_01_side.pac", .90f},
    {"data\\plot_beton_02_side.pac", .90f},   {"data\\plot_beton_03_side.pac", .90f},
    {"data\\plot_beton_0301_side.pac", .90f}, {"data\\plot_beton_0302_side.pac", .90f},
    {"data\\plot_beton_01_top.pac", .90f},    {"data\\plot_beton_03_top.pac", .90f},
    {"data\\sloupek_bily_orezly_.pac", 0}};
inline unsigned StockPBRIdentity(std::string_view texture)
{
    for (unsigned i = 0; i < std::size(StockPBRMaterials); ++i)
        if (texture == StockPBRMaterials[i].texture)
            return 64 + i;
    return 0;
}
inline float PBRMaterial(std::string_view texture, int legacyPower)
{
    const auto id = StockPBRIdentity(texture);
    if (!id)
        return 0;
    if (const float roughness = StockPBRMaterials[id - 64].roughness; roughness > 0)
        return roughness;
    // Existing painted-signpost lobe-width hint, not a measured conversion.
    return legacyPower > 0 ? std::clamp(std::pow(2.f / (float(legacyPower) + 2.f), .25f), .5f, .65f) : .6f;
}
} // namespace Poseidon::vk
