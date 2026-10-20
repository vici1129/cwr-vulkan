#pragma once
#include <array>
#include <cmath>
#include <PoseidonVK/ShapeGeometryVK.hpp>
namespace Poseidon::vk
{
struct TerrainReceiverRange { uint32_t begin, end; bool dry; };
inline std::vector<TerrainReceiverRange> TerrainReceiverRanges(const ShapeGeometry& geometry, float dryHeight)
{
    std::vector<TerrainReceiverRange> ranges;
    for (uint32_t i=0;i<geometry.indices.size();i+=3)
    {
        bool dry=true;
        for (unsigned k=0;k<3;++k)
        {
            const auto& v=geometry.vertices[geometry.indices[i+k]];
            dry=dry && std::isfinite(v.position[1]) && v.position[1]>dryHeight &&
                std::isfinite(v.normal[0]) && std::isfinite(v.normal[1]) && std::isfinite(v.normal[2]) &&
                v.normal[1]>0;
        }
        if (!ranges.empty() && ranges.back().dry==dry) ranges.back().end=i+3;
        else ranges.push_back({i,i+3,dry});
    }
    return ranges;
}
// Original landscape inputs only. Reject the entire potential coastal band,
// invalid normals and non-palette values; no light/color inversion or guessing.
inline std::array<float, 8> TerrainReceiverMaterial(float height, float dryHeight,
    std::array<float, 3> normal, std::array<float, 3> tint)
{
    if (!std::isfinite(height) || height <= dryHeight) return {};
    float norm = 0;
    for (int k=0;k<3;++k)
    {
        if (!std::isfinite(normal[k]) || !std::isfinite(tint[k]) || tint[k]<0 || tint[k]>2) return {};
        norm += normal[k]*normal[k];
    }
    if (!std::isfinite(norm) || norm<1e-12f || normal[1]<=0) return {};
    const float inv=1/std::sqrt(norm);
    return {normal[0]*inv,normal[1]*inv,normal[2]*inv,1,tint[0],tint[1],tint[2],1};
}
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
