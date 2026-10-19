#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>

namespace Poseidon::vk
{
// Inspected stock sections, not a name/prefix heuristic. These images contain
// rough dielectric surfaces; mixed window/metal atlases and unsupported LOD
// substitutions remain legacy. See docs/pbr-coverage.md for pixels/model evidence.
inline float PBRMaterial(std::string_view texture, int legacyPower)
{
    if (texture == "data\\skala_piskovec2.pac") return .85f;
    // Dedicated weathered roof tiles (including their rotated stock variant)
    // and dark exterior wood. Unmeasured dielectric defaults, not glossy glaze,
    // varnish or metal; existing baked texture detail is left untouched.
    if (texture == "data\\strecha_tasky_vln.pac" || texture == "data\\strecha_tasky_vln90.pac") return .90f;
    if (texture == "data\\prkna_dark.pac") return .85f;
    if (texture == "data\\telegraf_sloup.pac" || texture == "data\\prkna_dalsi.pac") return .85f;
    if (texture == "data\\tasky_tmavsi.pac") return .90f;
    // Stone/mortar with a fired-clay cap; worn plaster exposing brick; weathered
    // concrete panels, pillars and caps. The closed barn's rough timber doors
    // share the conservative matte response of its plaster/brick facade. No
    // glass, varnish or exposed metal is promoted by these identities.
    if (texture == "data\\kamennazed.pac" || texture == "data\\omlacena_bocni_zed.pac" ||
        texture == "data\\stodola_f.pac" || texture == "data\\stodola_f2.pac" || texture == "data\\stodola_s.pac" ||
        texture == "data\\betondlouhastenabez.pac" ||
        texture == "data\\betonpoprask_side.pac" || texture == "data\\betonpoprask_top.pac" ||
        texture == "data\\plot_beton_01_side.pac" || texture == "data\\plot_beton_02_side.pac" ||
        texture == "data\\plot_beton_03_side.pac" || texture == "data\\plot_beton_0301_side.pac" ||
        texture == "data\\plot_beton_0302_side.pac" || texture == "data\\plot_beton_01_top.pac" ||
        texture == "data\\plot_beton_03_top.pac") return .90f;
    if (texture != "data\\sloupek_bily_orezly_.pac") return 0;
    // A lobe-width hint only, NOT power == roughness or a measured conversion.
    // Keep even high-power legacy materials rough; absent specular uses .6.
    return legacyPower > 0 ? std::clamp(std::pow(2.f / (float(legacyPower) + 2.f), .25f), .5f, .65f) : .6f;
}
}
