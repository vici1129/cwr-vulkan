#pragma once
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <cstddef>

namespace Poseidon::vk
{
// Packed above the three sampler bits in the shared ray material.
// These describe available inputs, not a physical/PBR material classification.
constexpr uint32_t RayDiffuse = 8, RayVertexNormals = 16, RayTerrainUV = 32;
constexpr uint32_t RayNormalMap = 64, RayRoughnessMap = 128;
inline uint32_t RaySurfaceFlags(int flags)
{
    const auto spec = render::SplitLegacy(flags);
    const bool diffuse = SupportedShapeSpec(spec) &&
                         !(flags & (DisableSun | IsColored | IsLight | IsAlpha | IsAlphaFog | SpecularTexture));
    return RayVertexNormals | (diffuse ? RayDiffuse : 0);
}
static_assert(sizeof(ShapeVertex) == 32 && offsetof(ShapeVertex, normal) == 12 &&
              offsetof(ShapeVertex, uv) == 24);
} // namespace Poseidon::vk
