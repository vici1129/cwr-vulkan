#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>

namespace Poseidon::vk
{
// Deliberately tiny stock-texture allowlist: rock and painted signpost surface.
// Neither a whole building/vehicle atlas nor a bare-metal classification.
inline float PBRMaterial(std::string_view texture, int legacyPower)
{
    if (texture == "data\\skala_piskovec2.pac") return .85f;
    if (texture != "data\\sloupek_bily_orezly_.pac") return 0;
    // A lobe-width hint only, NOT power == roughness or a measured conversion.
    // Keep even high-power legacy materials rough; absent specular uses .6.
    return legacyPower > 0 ? std::clamp(std::pow(2.f / (float(legacyPower) + 2.f), .25f), .5f, .65f) : .6f;
}
}
