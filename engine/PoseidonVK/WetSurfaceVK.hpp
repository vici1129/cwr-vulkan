#pragma once
#include <array>
#include <string_view>

namespace Poseidon::vk
{
// Stock CfgSurfaces identities, not color/brightness heuristics. Unknown and
// mixed grass ground, foliage, clothing and ordinary object atlases are excluded.
inline std::array<float, 3> WetSurface(std::string_view surface)
{
    if (surface == "silnice*" || surface == "asfalt*" || surface == "rw*" || surface == "bt??????")
        return {.16f, .10f, 48.f};
    if (surface == "cesta*" || surface == "pol" || surface == "bah")
        return {.13f, .035f, 24.f};
    if (surface == "sh??????" || surface == "s???????")
        return {.12f, .065f, 36.f};
    if (surface == "pi??????" || surface == "ps??????")
        return {.10f, .02f, 18.f};
    return {};
}
}
