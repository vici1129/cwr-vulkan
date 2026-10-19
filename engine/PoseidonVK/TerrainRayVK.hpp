#pragma once
#include <array>
#include <cmath>
namespace Poseidon::vk
{
// Same bilinear UV jitter as LandscapeRender, evaluated in one parent land cell.
// Clamped terrain tiles must not share UV vertices across texture-cell seams.
inline std::array<float, 2> TerrainRayUV(float x, float z, float cellX, float cellZ, bool clamp,
                                         const std::array<float, 4>& u, const std::array<float, 4>& v)
{
    const float fx = x - cellX, fz = z - cellZ;
    auto offset = [&](const auto& a) { return std::lerp(std::lerp(a[0], a[1], fx), std::lerp(a[2], a[3], fx), fz); };
    return {(clamp ? fx : x) + offset(u), (clamp ? fz : z) + offset(v)};
}
// LandscapeRender LOD0's original central-difference normal, negated because
// the engine stores inward normals. No (0,1,0) slope placeholder.
inline std::array<float, 3> TerrainRayNormal(float left, float right, float back, float front, float grid)
{
    const float x = left - right, z = back - front, inv = 1.f / std::sqrt(x * x + grid * grid + z * z);
    return {x * inv, grid * inv, z * inv};
}
} // namespace Poseidon::vk
