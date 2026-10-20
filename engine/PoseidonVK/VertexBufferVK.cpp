#include <PoseidonVK/VertexBufferVK.hpp>
#include <PoseidonVK/EngineVK.hpp>
#include <cstring>
#include <Poseidon/World/Terrain/LandscapeShared.hpp>

namespace Poseidon
{
namespace
{
bool ImmutableBuffer(const Shape& shape, VBType type)
{
    // RemmemberShadow builds these once after terrain fitting, and retires
    // them when its cache entry changes. Other small/dynamic buffers still TL.
    return type == VBStatic || type == VBBigDiscardable ||
           (type == VBSmallDiscardable && (shape.Special() & IsShadow));
}
}
VertexBufferVK::VertexBufferVK(vk::VulkanContext& context, const Shape& shape, VBType type) : _context(context)
{
    if (!ImmutableBuffer(shape, type))
        throw std::logic_error("Vulkan Shape: only immutable geometry and cached shadow buffers are supported");
    auto geometry = vk::ExtractShapeGeometry(shape);
    _sections = std::move(geometry.sections);
    _buffers = context.UploadMesh(geometry.vertices.data(), geometry.vertices.size() * sizeof(vk::ShapeVertex),
                                  geometry.indices.data(), geometry.indices.size() * sizeof(VertexIndex));
}

void VertexBufferVK::Update(const Shape& shape, bool dynamic)
{
    if (!vk::NeedsShapeSnapshot(dynamic, bufferDirty))
        return;
    // Damage/animation can invalidate a formerly immutable shared Shape. Never
    // overwrite its allocation: raster frames and cached BLAS retain the old
    // shared snapshot until their fences retire it.
    auto geometry = vk::ExtractShapeGeometry(shape);
    const auto vertexBytes = geometry.vertices.size() * sizeof(vk::ShapeVertex);
    const auto indexBytes = geometry.indices.size() * sizeof(VertexIndex);
    // Animate/Deanimate may mark a shared shape dirty even when the restored
    // data is identical. Its host-visible buffers are immutable GPU inputs, so
    // comparing them is safe even while older frames read the snapshot.
    if (_buffers->vertices.size == vertexBytes && _buffers->indices.size >= indexBytes &&
        _buffers->indices.size < indexBytes + 4 &&
        std::memcmp(_buffers->vertices.mapped, geometry.vertices.data(), vertexBytes) == 0 &&
        std::memcmp(_buffers->indices.mapped, geometry.indices.data(), indexBytes) == 0)
    {
        _sections = std::move(geometry.sections);
        bufferDirty = false;
        return;
    }
    auto buffers = _context.UploadMesh(geometry.vertices.data(), geometry.vertices.size() * sizeof(vk::ShapeVertex),
                                       geometry.indices.data(), geometry.indices.size() * sizeof(VertexIndex));
    _sections = std::move(geometry.sections);
    _buffers = std::move(buffers);
    _terrainReceivers.clear();
    _terrainReceiversReady = false;
    bufferDirty = false;
}

const std::vector<vk::TerrainReceiverRange>& VertexBufferVK::TerrainReceivers()
{
    // Only the explicit landscape-ground receiver draw requests these ranges.
    // Ordinary objects, water and GI-Off never scan triangles or allocate ranges.
    // Read the retained immutable snapshot; unchanged dirty notifications keep it.
    if (!_terrainReceiversReady)
    {
        _terrainReceivers = vk::TerrainReceiverRanges(
            {static_cast<const vk::ShapeVertex*>(_buffers->vertices.mapped),
             size_t(_buffers->vertices.size / sizeof(vk::ShapeVertex))},
            {static_cast<const VertexIndex*>(_buffers->indices.mapped),size_t(_sections.back().end)},maxTide+maxWave);
        _terrainReceiversReady = true;
    }
    return _terrainReceivers;
}

VertexBuffer* EngineVK::CreateVertexBuffer(const Shape& shape, VBType type)
{
    if (!ImmutableBuffer(shape, type))
    {
        LOG_DEBUG(Graphics,
                  "Vulkan: declining dynamic/discardable vertex buffer type {} ({} vertices); use engine software "
                  "transformation",
                  int(type), shape.NVertex());
        return nullptr; // Engine contract: unavailable hardware buffer, not a successful draw.
    }
    return new VertexBufferVK(_vk, shape, type);
}
} // namespace Poseidon
