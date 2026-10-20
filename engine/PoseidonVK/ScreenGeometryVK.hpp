#pragma once
#include <PoseidonVK/ScreenBatchVK.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <cmath>
#include <stdexcept>

namespace Poseidon::vk
{
// Sparse clipped faces can reference a few vertices of a very large TL table.
// Retain indexed sharing/order but upload only those referenced vertices. The
// scratch map is initialized once per BeginMesh, then reset only at touched IDs.
inline void CompactSoftwareIndices(std::span<const uint32_t> indices,std::span<uint32_t> remap,
    std::vector<uint32_t>& sources,std::vector<uint32_t>& drawIndices)
{
    for (auto index : indices)
        if (index>=remap.size()) throw std::out_of_range("Vulkan software face vertex exceeds the transformed table");
    sources.clear(); drawIndices.clear();
    for (auto index : indices)
    {
        auto& mapped=remap[index];
        if (mapped==UINT32_MAX) { mapped=uint32_t(sources.size()); sources.push_back(index); }
        drawIndices.push_back(mapped);
    }
    for (auto index : sources) remap[index]=UINT32_MAX;
}
// Preserve engine reciprocal-W, UV and packed ARGB colors. A world-space
// ribbon (e.g. a tracer passing the camera) can have an endpoint behind the
// eye. Keep its negative W so homogeneous GPU clipping handles the crossing;
// rejecting it crashes combat, and abs/clamping would move it in front.
inline ScreenVertex ScreenGeometry(const Vertex2DAbs& vertex, int width, int height)
{
    if (width <= 0 || height <= 0 || !std::isfinite(vertex.w) || vertex.w == 0)
        throw std::invalid_argument("Vulkan screen vertex requires an extent and finite nonzero reciprocal-W");
    const float w = 1 / vertex.w;
    return {{(2 * vertex.x / width - 1) * w, (2 * vertex.y / height - 1) * w, vertex.z * w, w},
            {vertex.u, vertex.v},
            {float((vertex.color >> 16) & 255) / 255, float((vertex.color >> 8) & 255) / 255,
             float(vertex.color & 255) / 255, float((vertex.color >> 24) & 255) / 255}};
}
} // namespace Poseidon::vk
