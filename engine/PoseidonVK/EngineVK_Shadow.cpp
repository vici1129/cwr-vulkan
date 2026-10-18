#include <PoseidonVK/EngineVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <algorithm>
#include <cmath>
#include <PoseidonVK/LocalShadowVK.hpp>

namespace Poseidon
{
RString EngineVK::SetLocalShadows(RString mode)
{
    std::string status = _vk.SetLocalShadows(static_cast<const char*>(mode));
    if (std::string_view(static_cast<const char*>(mode)) == "status")
        for (int i = 0; i < _localShadowCount; ++i)
        {
            const auto& light = _localShadowDescriptions[i];
            status += " light" + std::to_string(i) + "=" + std::to_string(light.pos.X()) + "," +
                      std::to_string(light.pos.Y()) + "," + std::to_string(light.pos.Z()) +
                      " dir=" + std::to_string(light.dir.X()) + "," + std::to_string(light.dir.Y()) + "," +
                      std::to_string(light.dir.Z());
        }
    return status.c_str();
}

Light* EngineVK::LocalShadowLight(int slot) const
{
    return _vk.LocalShadowsEnabled() && slot >= 0 && slot < _localShadowCount
               ? static_cast<Light*>(_localShadowLights[slot])
               : nullptr;
}

int EngineVK::PrepareLocalShadows()
{
    if (!_vk.FrameOpen() || !_shadowWorld || !_vk.LocalShadowsEnabled() || !GScene || !GScene->GetCamera() ||
        GScene->MainLight()->NightEffect() <= .001f)
    {
        _localShadowCount = 0;
        return 0;
    }
    struct Candidate
    {
        Light* light;
        LightDescription desc;
        float score;
        int order;
    };
    std::vector<Candidate> candidates;
    const auto* camera = GScene->GetCamera();
    const auto& active = GScene->ActiveLights();
    // Scene insertion order is the deterministic tie breaker, not pointer order
    // or the brightness sort used by the per-object material light lists.
    for (int i = 0; i < GScene->NLights(); ++i)
    {
        auto* light = GScene->GetLight(i);
        if (!light || !light->IsOn())
            continue;
        bool used = false;
        for (int j = 0; j < active.Size(); ++j)
            if (active[j] == light)
                used = true;
        if (!used)
            continue;
        LightDescription desc{};
        light->GetDescription(desc);
        if (desc.type != LTSpotLight || !std::isfinite(desc.phi) || !std::isfinite(desc.startAtten) ||
            desc.startAtten <= .01f || desc.dir.SquareSize() < .5f || !std::isfinite(desc.dir.SquareSize()))
            continue;
        const float range = std::min(vk::LocalShadowMaxRange, desc.startAtten * 10.f);
        const auto relative = desc.pos - camera->Position();
        const float distance2 = relative.SquareSize();
        if (!std::isfinite(distance2) || distance2 > Square(range + 80.f))
            continue;
        // Conservative cone bounding sphere against the camera frustum. This
        // also keeps an off-screen headlight whose illuminated ground is visible.
        const auto center = relative + desc.dir.Normalized() * (range * .5f);
        const float radius = range * .55f;
        const float z = center * camera->Direction();
        if (z + radius <= camera->ClipNear() ||
            std::abs(center * camera->DirectionAside()) >
                std::max(z, 0.f) * camera->Left() + radius * (1 + camera->Left()) ||
            std::abs(center * camera->DirectionUp()) > std::max(z, 0.f) * camera->Top() + radius * (1 + camera->Top()))
            continue;
        float score = std::max({desc.diffuse.R(), desc.diffuse.G(), desc.diffuse.B()}) * Square(desc.startAtten) /
                      std::max(distance2, 25.f);
        if (!std::isfinite(score) || score <= .001f)
            continue;
        for (int slot = 0; slot < _localShadowCount; ++slot)
            if (_localShadowLights[slot] == light)
                score *= 1.3f; // replacement hysteresis
        candidates.push_back({light, desc, score, i});
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b)
                     { return a.score != b.score ? a.score > b.score : a.order < b.order; });
    if (candidates.size() > 2)
        candidates.resize(2);
    // Retain slot order as the observer crosses the two headlights' midpoint.
    if (candidates.size() == 2 && _localShadowLights[0] == candidates[1].light)
        std::swap(candidates[0], candidates[1]);
    _localShadowCount = int(candidates.size());
    for (int slot = 0; slot < 2; ++slot)
    {
        _localShadowLights[slot] = slot < _localShadowCount ? candidates[slot].light : nullptr;
        if (slot < _localShadowCount)
            _localShadowDescriptions[slot] = candidates[slot].desc;
    }
    return _localShadowCount;
}

bool EngineVK::LocalShadowCaster(int slot, Vector3Par position, float radius) const
{
    if (slot < 0 || slot >= _localShadowCount)
        return false;
    const auto& light = _localShadowDescriptions[slot];
    const auto relative = position - light.pos;
    const float range = std::min(vk::LocalShadowMaxRange, light.startAtten * 10.f);
    if (relative.SquareSize() > Square(range + radius))
        return false;
    const float along = relative * light.dir.Normalized();
    const float halfAngle = std::clamp(light.phi * .5f, .21f, .7f);
    return along >= -radius && along <= range + radius &&
           relative.SquareSize() - along * along <=
               Square(std::max(along, 0.f) * std::tan(halfAngle) + radius / std::cos(halfAngle));
}

void EngineVK::RenderLocalShadowDepth(int slot, const ShadowCasterSet& casters)
{
    if (!_vk.LocalShadowsEnabled() || slot < 0 || slot >= _localShadowCount)
        return;
    const auto& light = _localShadowDescriptions[slot];
    const auto camera = GScene->GetCamera()->Position();
    const auto p = light.pos - camera, d = light.dir.Normalized();
    const float range = std::min(vk::LocalShadowMaxRange, light.startAtten * 10.f);
    const auto matrix = vk::LocalShadowMatrix({p.X(), p.Y(), p.Z()}, {d.X(), d.Y(), d.Z()},
                                              std::clamp(light.phi * .5f, .21f, .7f), range);
    _vk.SetLocalShadowMatrix(slot, matrix, range);
    std::vector<vk::ShadowAlphaBatch> batches;
    for (int i = 0; i < casters.alphaBatchCount; ++i)
    {
        const auto& batch = casters.alphaBatches[i];
        auto* texture = dynamic_cast<TextureVK*>(batch.texture);
        batches.push_back(
            {texture ? texture->Image(_vk) : nullptr, uint32_t(batch.firstVertex), uint32_t(batch.vertexCount)});
    }
    std::vector<float> solid;
    if (casters.solidVertexCount > 0)
        solid.assign(casters.solidXYZ, casters.solidXYZ + size_t(casters.solidVertexCount) * 3);
    // Sample the existing terrain surface only inside this light's bounded cone.
    // No new terrain cache or animation path; the no-cull depth pipeline includes hills
    // that face away from the observer and would be absent from the screen mesh.
    if (auto* land = GScene->GetLandscape())
    {
        const float grid = std::max(3.125f, land->GetTerrainGrid());
        const int x0 = int(std::floor((light.pos.X() - range) / grid)),
                  x1 = int(std::ceil((light.pos.X() + range) / grid));
        const int z0 = int(std::floor((light.pos.Z() - range) / grid)),
                  z1 = int(std::ceil((light.pos.Z() + range) / grid));
        auto vertex = [&](float x, float z) { return Vector3(x, land->SurfaceY(x, z), z); };
        auto triangle = [&](Vector3Par a, Vector3Par b, Vector3Par c)
        {
            for (const auto* v : {&a, &b, &c})
            {
                const auto r = *v - camera;
                solid.insert(solid.end(), {r.X(), r.Y(), r.Z()});
            }
        };
        for (int z = z0; z < z1; ++z)
            for (int x = x0; x < x1; ++x)
            {
                const auto center = vertex((x + .5f) * grid, (z + .5f) * grid);
                if (!LocalShadowCaster(slot, center, grid * 2))
                    continue;
                const auto a = vertex(x * grid, z * grid), b = vertex((x + 1) * grid, z * grid);
                const auto c = vertex(x * grid, (z + 1) * grid), e = vertex((x + 1) * grid, (z + 1) * grid);
                triangle(a, b, c);
                triangle(b, e, c);
            }
    }
    _vk.RenderShadowDepth(matrix.data(), 1, vk::LocalShadowResolution, solid,
                          {casters.alphaXYZUV, size_t(casters.alphaVertexCount) * 5}, batches, {}, 0, {}, slot);
}

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
        // supplies valley/ridge occlusion using the actual landscape vertices
        // and SurfaceY's 01--10 diagonal. Resampling onto an unrelated 8 m grid
        // can put the caster above the visible surface at finer terrain detail.
        // Kept in the Vulkan path and allocated only for this optional effect.
        auto* land = GScene->GetLandscape();
        const auto position = GScene->GetCamera()->Position();
        const float radius = std::min(600.f, splits[count - 1] + 100.f);
        const float grid = land->GetTerrainGrid();
        // Reuse the immutable upload across small movements even at 3.125 m
        // detail. The margin retains the full radius until the next anchor.
        const int anchorCells = std::max(1, int(25.f / grid));
        const int cx = int(std::floor(position.X() / (grid * anchorCells))) * anchorCells;
        const int cz = int(std::floor(position.Z() / (grid * anchorCells))) * anchorCells;
        const int cells = int(std::ceil(radius / grid)) + anchorCells;
        const auto key = vk::ShadowTerrainKey(cx, cz, grid, cells, land->GetTerrainGrid());
        if (_volumeLandscape != land || _volumeTerrainKey != key)
        {
            _volumeTerrain.clear();
            const uint32_t stride = 2 * cells + 1;
            _volumeTerrain.reserve(size_t(stride) * stride * 3);
            for (int z = cz - cells; z <= cz + cells; ++z)
                for (int x = cx - cells; x <= cx + cells; ++x)
                    _volumeTerrain.insert(_volumeTerrain.end(),
                                          {(x - cx) * grid, land->ClippedData(z, x), (z - cz) * grid});
            // Share the actual grid vertices rather than transforming six
            // copies per cell in every cascade. The terrain pipeline is
            // two-sided, so reverse faces are unnecessary too.
            std::vector<uint32_t> indices;
            indices.reserve(size_t(stride - 1) * (stride - 1) * 6);
            for (uint32_t z = 0; z < stride - 1; ++z)
                for (uint32_t x = 0; x < stride - 1; ++x)
                {
                    const auto a = z * stride + x;
                    indices.insert(indices.end(), {a, a + 1, a + stride, a + 1, a + stride + 1, a + stride});
                }
            _volumeTerrainMesh = _vk.UploadMesh(_volumeTerrain.data(), _volumeTerrain.size() * sizeof(float),
                                                indices.data(), indices.size() * sizeof(uint32_t));
            _volumeTerrainIndices = uint32_t(indices.size());
            _volumeLandscape = land;
            _volumeTerrainKey = key;
        }
        terrain = _volumeTerrainMesh;
        terrainOffset = {cx * grid - position.X(), -position.Y(), cz * grid - position.Z()};
    }
    _vk.RenderShadowDepth(matrices, count, resolution, solid,
                          {casters.alphaXYZUV, size_t(casters.alphaVertexCount) * 5}, batches,
                          terrain, terrain ? _volumeTerrainIndices : 0, terrainOffset);
}
} // namespace Poseidon
