#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>
#include <iterator>

namespace Poseidon::vk
{
struct QualifiedMaterial
{
    unsigned identity = 0, family = 0;
    float roughness = 0;
    explicit operator bool() const { return identity != 0; }
};
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
    {"data\\tasky_p90.pac", .90f, 3}, {"data\\budova2_stit2.pac", .90f, 5},
    // Original Nogova stone and worn masonry. Entire decoded sources are rough
    // dielectrics; shared users still require ordinary lighting/geometry/opacity.
    {"o\\skal\\kamen02.pac", .85f, 2}, {"o\\skal\\kamen04.pac", .85f, 2},
    {"o\\skal\\kamen05.pac", .85f, 2}, {"o\\hous\\zidka01.pac", .90f, 6},
    {"data\\2_sloup_cihl.pac", .90f, 6}, {"data\\2_sloup_cihl_n.pac", .90f, 6},
    // Dedicated bark/stacked-log sources only. Canopies, corrugated roofing,
    // hinged doors and mixed photographic end/support atlases stay legacy.
    {"data\\kmen_borovice_04.pac", .85f, 4}, {"o\\tree\\javor01_kr3.pac", .85f, 4},
    {"o\\hous\\hrmd_kr.pac", .85f, 4}, {"o\\hous\\hrmd_top.pac", .85f, 4},
    {"o\\hous\\hrmd_zbk.pac", .85f, 4},
    // Original separately sectioned fence foundations/caps and blank plaster.
    // Adjacent wire, hardware, apertures and window-bearing atlases stay legacy.
    {"data\\podezdivka1.pac", .90f, 7}, {"data\\podezdivka2.pac", .90f, 6},
    {"data\\2_sloup_beton.pac", .90f, 7}, {"data\\vrsek_zidky.pac", .90f, 7},
    {"data\\tasky_p.pac", .90f, 3}, {"data\\tasky_hnus.pac", .90f, 3},
    {"data\\tasky_hnus90.pac", .90f, 3}, {"data\\blok_a_back.pac", .90f, 5},
    {"data\\blok_a_rantl_top.pac", .90f, 5}, {"data\\komin_vetsi_side_01.pac", .90f, 5},
    {"data\\ujezd_up.pac", .90f, 5},
    // Dedicated original timber, rough roofing and plain building finishes.
    // Hardware/window/vent-bearing neighbor atlases do not inherit these keys.
    {"data\\futra.pac", .85f, 4}, {"data\\kostel_t_tramy_podel.pac", .85f, 4},
    {"o\\hous\\drevo02.pac", .85f, 4}, {"o\\hous\\drevo_modre.pac", .85f, 4},
    {"o\\hous\\strecha02.pac", .90f, 3}, {"o\\hous\\strecha_tasky.pac", .90f, 3},
    {"o\\hous\\tasky_tmavsi.pac", .90f, 3}, {"o\\hous\\dumpod_beton.pac", .90f, 7},
    {"o\\hous\\dumpod_podez.pac", .90f, 6}, {"o\\hous\\zalchat_beton.pac", .90f, 7},
    {"o\\hous\\zalchat_strop.pac", .85f, 4}, {"o\\hous\\stenain01.pac", .90f, 5},
    {"o\\hous\\panel04.pac", .90f, 7}, {"o\\hous\\panel_inter02.pac", .90f, 5},
    // Dedicated original bark on separately modeled trunks/branches. Repeating
    // UVs sample this same physical class; canopy/impostor and authored vertices
    // keep their existing gates. Mixed photo/background and cut-end atlases stay
    // untyped. These coefficients are the existing unmeasured wood approximation.
    {"data\\kmen2.pac", .85f, 4}, {"data\\kmen_borovice.pac", .85f, 4},
    {"data\\kura3.pac", .85f, 4}, {"data\\kura_jablon_asi_ne.pac", .85f, 4},
    {"data\\lesni_kmen_smrku.pac", .85f, 4}, {"data\\n_strom_01_kura.pac", .85f, 4},
    {"data\\n_strom_lipa_kmen.pac", .85f, 4}, {"o\\tree\\akat01_krd2.pac", .85f, 4},
    {"o\\tree\\borovice02c.pac", .85f, 4}, {"o\\tree\\dd_les_kura03.pac", .85f, 4},
    {"o\\tree\\dub_kura_fx.pac", .85f, 4}, {"o\\tree\\smrkur.pac", .85f, 4},
    // Original dedicated sandstone, castle foundation stone and separately modeled stone
    // memorial sides/base. Mixed grave/inscription/soil atlases stay untyped.
    {"data\\piskovec.pac", .85f, 2}, {"data\\skala_piskovec.pac", .90f, 6},
    {"data\\skala_piskovec3.pac", .85f, 2}, {"data\\bozi_muka_sokl.pac", .90f, 6},
    {"data\\hrobecek2_side.pac", .90f, 6}, {"data\\hrobecek2_side2.pac", .90f, 6},
    {"data\\hrobecek2_side3.pac", .90f, 6},
    // Dedicated original overlapping roof tiles, including shared gate caps.
    // Chimney apertures, metal/coating ambiguity and mixed roof atlases do not
    // inherit these identities. Original lighting/capture gates still apply.
    {"data\\kostel_strecha.pac", .90f, 3}, {"data\\strecha_tasky.pac", .90f, 3},
    {"o\\hous\\hasicakstrecha.pac", .90f, 3},
    // Original weathered concrete marker/barrier blocks. The separate rail,
    // rusty pipes and mixed sign coatings do not inherit this identity.
    {"data\\patnik_new.pac", .90f, 7},
    // Original bell-station mineral body. Its separate bell/cutouts and all
    // neighboring facade sources retain their original contracts.
    {"data\\zvonicka_side.pac", .90f, 6},
    // Original exposed concrete barriers, military structure faces and thick
    // painted firing panels. Dark siblings, doors and hardware stay separate;
    // original per-triangle authored-lighting gates remain independent.
    {"data\\beton_universal.pac", .90f, 7},
    {"data\\beton_preklad_opak.pac", .90f, 7},
    {"data\\panel_maskac_opak.pac", .90f, 7}};
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
