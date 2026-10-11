#include <PoseidonVK/EngineVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <algorithm>
#include <cmath>

namespace Poseidon
{
void EngineVK::SetShadowMapsEnabled(bool enabled)
{
    _vk.FlushScreenBatch();
    _shadowTuning.enabled = enabled;
    _vk.ResetShadowState();
}

void EngineVK::SetShadowMapTuning(const ShadowMapTuning& tuning)
{
    // The existing developer controls validate their values too; keep the
    // backend safe for direct callers and reject before changing live state.
    if (tuning.cascadeCount < 1 || tuning.cascadeCount > 4 || tuning.resolution < 128 || tuning.resolution > 4096 ||
        tuning.omniCount < 0 || tuning.omniCount > 4 || !std::isfinite(tuning.darkness) || tuning.darkness < 0 ||
        tuning.darkness > 1 || !std::isfinite(tuning.biasBase) || tuning.biasBase < 0 ||
        !std::isfinite(tuning.fadeRange) || tuning.fadeRange <= 0 || !std::isfinite(tuning.distanceCoef) ||
        tuning.distanceCoef <= 0 || tuning.distanceCoef > 1 || !std::isfinite(tuning.splitCoef) ||
        tuning.splitCoef < 0 || tuning.splitCoef > 1 || !std::isfinite(tuning.omniCoef0) ||
        !std::isfinite(tuning.omniCoef1) || tuning.omniCoef0 <= 0 || tuning.omniCoef1 < tuning.omniCoef0 ||
        tuning.omniCoef1 > 1 || !std::isfinite(tuning.casterLodBias) || tuning.casterLodBias < 1)
        throw std::invalid_argument("Vulkan shadow-map tuning is outside the supported range");
    _vk.FlushScreenBatch();
    _shadowTuning = tuning;
    _vk.ResetShadowState();
}

void EngineVK::SetShadowMapSunFactor(float factor)
{
    _shadowSunFactor = std::isfinite(factor) ? std::clamp(factor, 0.f, 1.f) : 0.f;
    if (_shadowSunFactor <= 0.01f)
        _vk.ResetShadowState();
}

void EngineVK::RenderShadowDepthScene(const float* matrices, const float* splits, const float* forward, int count,
                                      int omniCount, int resolution, const ShadowCasterSet& casters)
{
    if (!_vk.FrameOpen() || !NeedsSunShadowDepth())
        return;
    if (!matrices || !splits || !forward || count < 1 || count > 4 || casters.solidVertexCount < 0 ||
        casters.alphaVertexCount < 0 || casters.alphaBatchCount < 0)
        throw std::invalid_argument("Vulkan invalid shadow caster set");
    std::vector<vk::ShadowAlphaBatch> batches;
    batches.reserve(casters.alphaBatchCount);
    for (int i = 0; i < casters.alphaBatchCount; ++i)
    {
        const auto& batch = casters.alphaBatches[i];
        auto* texture = dynamic_cast<TextureVK*>(batch.texture);
        if (batch.texture && !texture)
            throw std::logic_error("Vulkan foreign shadow caster texture");
        if (batch.firstVertex < 0 || batch.vertexCount < 0 ||
            batch.firstVertex > casters.alphaVertexCount - batch.vertexCount)
            throw std::out_of_range("Vulkan shadow alpha range");
        batches.push_back(
            {texture ? texture->Image(_vk) : nullptr, uint32_t(batch.firstVertex), uint32_t(batch.vertexCount)});
    }
    vk::ShadowLighting state;
    std::copy_n(matrices, count * 16, state.matrices.begin());
    std::copy_n(splits, count, state.splits.begin());
    std::copy_n(forward, 3, state.forward.begin());
    state.control = {float(count), _shadowTuning.fadeRange, _shadowTuning.biasBase,
                     float(std::clamp(omniCount, 0, count))};
    state.strength = {1.f - _shadowSunFactor * (1.f - _shadowTuning.darkness), 1.f / resolution, 0, 0};
    _vk.SetShadowState(state);
    std::span<const float> solid{casters.solidXYZ, size_t(casters.solidVertexCount) * 3};
    std::shared_ptr<vk::MeshBuffers> terrain;
    std::array<float, 3> terrainOffset{};
    if (_vk.VolumetricScatteringEligible() && GScene && GScene->GetLandscape())
    {
        // The object CSM collection has no landscape triangles. A bounded patch
        // supplies valley/ridge occlusion using the existing terrain surface.
        // Kept in the Vulkan path and allocated only for this optional effect.
        auto* land = GScene->GetLandscape();
        const auto position = GScene->GetCamera()->Position();
        const float radius = std::min(600.f, splits[count - 1] + 100.f);
        const float grid = std::max(8.f, land->GetTerrainGrid());
        const int cx = int(std::floor(position.X() / grid)), cz = int(std::floor(position.Z() / grid));
        const int cells = int(std::ceil(radius / grid));
        const std::array<float, 4> key{float(cx), float(cz), grid, float(cells)};
        if (_volumeLandscape != land || _volumeTerrainKey != key)
        {
            _volumeTerrain.clear();
            auto vertex = [&](float x, float z) {
                return std::array<float, 3>{x - cx * grid, land->SurfaceY(x, z), z - cz * grid};
            };
            auto triangle = [&](const auto& a, const auto& b, const auto& c)
            {
                // Both sides: the ordinary solid CSM pipeline culls front faces.
                for (const auto* v : {&a, &b, &c, &c, &b, &a})
                    _volumeTerrain.insert(_volumeTerrain.end(), v->begin(), v->end());
            };
            for (int z = cz - cells; z < cz + cells; ++z)
                for (int x = cx - cells; x < cx + cells; ++x)
                {
                    const auto a = vertex(x * grid, z * grid), b = vertex((x + 1) * grid, z * grid);
                    const auto c = vertex(x * grid, (z + 1) * grid), d = vertex((x + 1) * grid, (z + 1) * grid);
                    triangle(a, b, c);
                    triangle(b, d, c);
                }
            const uint32_t unusedIndex = 0;
            _volumeTerrainMesh = _vk.UploadMesh(_volumeTerrain.data(), _volumeTerrain.size() * sizeof(float),
                                                &unusedIndex, sizeof(unusedIndex));
            _volumeLandscape = land;
            _volumeTerrainKey = key;
        }
        terrain = _volumeTerrainMesh;
        terrainOffset = {cx * grid - position.X(), -position.Y(), cz * grid - position.Z()};
    }
    _vk.RenderShadowDepth(matrices, count, resolution, solid,
                          {casters.alphaXYZUV, size_t(casters.alphaVertexCount) * 5}, batches,
                          terrain, terrain ? uint32_t(_volumeTerrain.size() / 3) : 0, terrainOffset);
}
} // namespace Poseidon
