#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/VertexBufferVK.hpp>
#include <PoseidonVK/RayTransformVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <set>

namespace Poseidon
{
void EngineVK::CollectRTAOScene()
{
    if (!_vk.RaySceneEnabled() || !GScene || !GScene->GetCamera() || !GScene->GetLandscape())
        return;
    const auto* camera = GScene->GetCamera();
    auto* land = GScene->GetLandscape();
    const auto position = camera->Position();
    const bool reflections = _vk.RTReflectionsEnabled();
    const bool shadows = _vk.RTShadowsEnabled() || reflections;
    const float radius = shadows ? 160.f : 80.f;
    const auto sunDirection = -GScene->MainLight()->Direction().Normalized();
    const float daylight = std::clamp(1.f - GScene->MainLight()->NightEffect(), 0.f, 1.f);
    _vk.SetRTSun({sunDirection.X(), sunDirection.Y(), sunDirection.Z(), sunDirection.Y() > .01f ? daylight : 0.f});
    if (reflections)
    {
        const auto* sun = GScene->MainLight();
        const auto eye = GetAccomodateEye();
        auto rgba = [](ColorVal c) { return std::array<float, 4>{c.R(), c.G(), c.B(), 1}; };
        auto zenith = sun->SkyColor() * eye, horizon = zenith;
        if (auto* sky = land->SkyTexture())
        {
            zenith = zenith * sky->GetPixel(0, 0, 0);
            horizon = horizon * sky->GetPixel(0, 1, 1);
        }
        _vk.SetReflectionLighting({rgba(sun->Ambient() * eye),
                                   rgba(sun->Diffuse() * eye),
                                   rgba(zenith),
                                   rgba(horizon),
                                   rgba(sun->SunSkyColor() * eye),
                                   rgba(_fogColor),
                                   _vk.EyeCoef(),
                                   {GScene->GetFogMinRange(),
                                    1.f / std::max(1.f, GScene->GetFogMaxRange() - GScene->GetFogMinRange()), 0, 0}});
    }
    uint32_t foliageCount = 0, terrainCount = 0;
    // Existing spatial buckets, independent of visibility and the CSM toggle.
    // AO fades out at 60m; the extra 20m covers short rays and nearby bounds.
    int x0, x1, z0, z1;
    ObjRadiusRectangle(x0, x1, z0, z1, position, position, radius);
    std::vector<vk::RTAOInstance> instances;
    std::set<const Object*> visited;
    const size_t objectLimit = shadows ? 800 : 1024;
    for (int z = z0; z <= z1 && instances.size() < objectLimit; ++z)
        for (int x = x0; x <= x1 && instances.size() < objectLimit; ++x)
        {
            const auto& objects = land->GetObjects(z, x);
            for (int i = 0; i < objects.Size(); ++i)
            {
                Object* object = objects[i];
                if (!object || !visited.insert(object).second || !object->Static() || !object->GetShape() ||
                    object->GetTotalDammage() > 0 || object->IsDammageDestroyed())
                    continue;
                if (object->GetObjSpecial() & (IsHidden | IsHiddenProxy | IsAlpha | IsTransparent | NoZBuf | NoZWrite))
                    continue;
                if (object->Position().Distance2(position) > Square(radius + object->GetRadius()))
                    continue;
                // Stable visual LOD, never collision hulls or proxy placeholders.
                // Terrain fitting and object animation invalidate the immutable vertices.
                auto* lod = object->GetShape();
                if (lod->NLevels() == 0 || object->IsAnimated(0))
                    continue;
                Shape* shape = lod->LevelOpaque(0);
                if (!shape || shape->IsAnimated() || shape->NVertex() == 0 ||
                    (shape->GetOrHints() & (ClipLandKeep | ClipLandOn)))
                    continue;
                vk::RTAOInstance instance;
                std::vector<vk::RTAORange> shadowOnly;
                instance.index16 = sizeof(VertexIndex) == 2;
                for (int s = 0; s < shape->NSections(); ++s)
                {
                    const auto& section = shape->GetSection(s);
                    const int flags = section.properties.Special();
                    if (flags & (IsAlpha | IsAlphaFog | IsHidden | IsHiddenProxy | IsShadow | IsAnimated | NoZBuf |
                                 NoZWrite | OnSurface | IsOnSurface | NoDropdown | IsLight))
                        continue;
                    auto* texture = section.properties.GetTexture();
                    if (auto* vkTexture = dynamic_cast<TextureVK*>(texture); vkTexture && vkTexture->IsDynamic())
                        continue;
                    const auto alpha = texture ? texture->GetAlphaClass() : AlphaStats::Opaque;
                    if (alpha == AlphaStats::Blend && (!shadows || !(flags & IsTransparent)))
                        continue;
                    vk::RTAORange candidate{uint32_t(s), 0};
                    candidate.sampler = vk::ShapeSampler(render::SplitLegacy(flags));
                    if (reflections)
                        if (auto* tex = dynamic_cast<TextureVK*>(texture); tex && !tex->IsDynamic())
                        {
                            candidate.color = tex->Image(_vk);
                            const auto& pixels = tex->Pixels();
                            _vk.PrepareRTAOAlpha(candidate.color, pixels.width, pixels.height, pixels.rgba.data());
                        }
                    bool extended = false;
                    if ((flags & IsTransparent) || alpha == AlphaStats::Cutout)
                    {
                        // Building walls often use alpha for real windows. Test those
                        // holes during traversal. Static foliage uses the same real alpha.
                        auto* vkTexture = dynamic_cast<TextureVK*>(texture);
                        if ((!shadows && object->GetDestructType() != DestructBuilding) || !vkTexture ||
                            vkTexture->IsDynamic())
                            continue;
                        extended = object->GetDestructType() != DestructBuilding || alpha == AlphaStats::Blend;
                        candidate.alpha = vkTexture->Image(_vk);
                        const auto& pixels = vkTexture->Pixels();
                        _vk.PrepareRTAOAlpha(candidate.alpha, pixels.width, pixels.height, pixels.rgba.data());
                        candidate.sampler = vk::ShapeSampler(render::SplitLegacy(flags));
                        candidate.cutoff = flags & IsTransparent ? 192.f / 255 : .5f;
                    }
                    (extended ? shadowOnly : instance.ranges).push_back(std::move(candidate));
                }
                if (instance.ranges.empty() && shadowOnly.empty())
                    continue;
                // Do not force a raster buffer for excluded foliage/effect models.
                if (!shape->GetVertexBuffer())
                    shape->ConvertToVBuffer(VBStatic);
                auto* buffer = dynamic_cast<VertexBufferVK*>(shape->GetVertexBuffer());
                if (!buffer)
                    continue;
                instance.mesh = buffer->Buffers();
                for (auto* ranges : {&instance.ranges, &shadowOnly})
                {
                    for (auto& range : *ranges)
                    {
                        const auto section = buffer->Sections().at(range.first);
                        range.first = section.begin;
                        range.count = section.end - section.begin;
                    }
                    std::erase_if(*ranges, [](const auto& range) { return range.count == 0; });
                }
                instance.transform = vk::RayInstanceTransform(object->Transform());
                if (!instance.ranges.empty())
                    instances.push_back(instance);
                if (!shadowOnly.empty() && instances.size() < objectLimit)
                {
                    instance.ranges = std::move(shadowOnly);
                    instance.mask = 2;
                    instance.kind = 1;
                    instances.push_back(std::move(instance));
                    ++foliageCount;
                }
                if (instances.size() >= objectLimit)
                    break;
            }
        }
    // Actual subdivided terrain samples and the same 01--10 cell diagonal as
    // Landscape::SurfaceY. Only local patches; height changes invalidate a patch.
    if (shadows)
    {
        const float grid = land->GetTerrainGrid();
        if (_rtLandscape != land || _rtTerrainGrid != grid)
        {
            _rtTerrainPatches.clear();
            _rtLandscape = land;
            _rtTerrainGrid = grid;
        }
        constexpr int cells = 16, stride = cells + 1;
        const float patchSize = cells * grid;
        std::set<std::pair<int, int>> used;
        for (int z = int(std::floor((position.Z() - radius) / patchSize));
             z <= int(std::floor((position.Z() + radius) / patchSize)); ++z)
            for (int x = int(std::floor((position.X() - radius) / patchSize));
                 x <= int(std::floor((position.X() + radius) / patchSize)) && instances.size() < 1024; ++x)
            {
                if (x < 0 || z < 0 || (x + 1) * cells >= land->GetTerrainRange() ||
                    (z + 1) * cells >= land->GetTerrainRange())
                    continue;
                const auto key = std::make_pair(x, z);
                used.insert(key);
                std::vector<float> heights;
                for (int j = 0; j <= cells; ++j)
                    for (int i = 0; i <= cells; ++i)
                        heights.push_back(land->SurfaceY((x * cells + i) * grid, (z * cells + j) * grid));
                auto& patch = _rtTerrainPatches[key];
                if (!patch.mesh || patch.heights != heights || patch.textured != reflections)
                {
                    // Same immutable position/normal/UV layout as native shapes.
                    std::vector<std::array<float, 8>> vertices;
                    for (int j = 0; j <= cells; ++j)
                        for (int i = 0; i <= cells; ++i)
                        {
                            float wx = (x * cells + i) * grid / land->GetLandGrid(),
                                  wz = (z * cells + j) * grid / land->GetLandGrid();
                            // Same landscape UV jitter at the four actual land-grid corners.
                            int lx = int(std::floor(wx)), lz = int(std::floor(wz));
                            float u[4], v[4];
                            land->GetRandomColor(lx, lz, u[0], v[0]);
                            land->GetRandomColor(lx + 1, lz, u[1], v[1]);
                            land->GetRandomColor(lx, lz + 1, u[2], v[2]);
                            land->GetRandomColor(lx + 1, lz + 1, u[3], v[3]);
                            auto interp = [&](float* a)
                            {
                                return std::lerp(std::lerp(a[0], a[1], wx - lx), std::lerp(a[2], a[3], wx - lx),
                                                 wz - lz);
                            };
                            vertices.push_back({float(i) * grid, heights[j * stride + i], float(j) * grid, 0, 1, 0,
                                                wx + interp(u), wz + interp(v)});
                        }
                    std::vector<uint32_t> indices;
                    std::map<TextureVK*, std::vector<uint32_t>> groups;
                    for (int j = 0; j < cells; ++j)
                        for (int i = 0; i < cells; ++i)
                        {
                            uint32_t a = j * stride + i;
                            TextureVK* texture = nullptr;
                            if (reflections)
                                texture = dynamic_cast<TextureVK*>(land->GetTexture(
                                    land->GetTexture(int((z * cells + j) * grid / land->GetLandGrid()),
                                                     int((x * cells + i) * grid / land->GetLandGrid()))));
                            auto& group = groups[texture];
                            group.insert(group.end(), {a, a + 1, a + stride, a + 1, a + stride + 1, a + stride});
                        }
                    patch.ranges.clear();
                    patch.textured = reflections;
                    for (auto& [texture, group] : groups)
                    {
                        vk::RTAORange range{uint32_t(indices.size()), uint32_t(group.size())};
                        if (texture && !texture->IsDynamic())
                        {
                            range.color = texture->Image(_vk);
                            const auto& pixels = texture->Pixels();
                            _vk.PrepareRTAOAlpha(range.color, pixels.width, pixels.height, pixels.rgba.data());
                        }
                        patch.ranges.push_back(range);
                        indices.insert(indices.end(), group.begin(), group.end());
                    }
                    patch.mesh = _vk.UploadMesh(vertices.data(), vertices.size() * sizeof(vertices[0]), indices.data(),
                                                indices.size() * sizeof(uint32_t));
                    patch.heights = std::move(heights);
                }
                vk::RTAOInstance instance;
                instance.mesh = patch.mesh;
                instance.index16 = false;
                instance.mask = 2;
                instance.kind = 2;
                instance.ranges = patch.ranges;
                Matrix4 model(MIdentity);
                model.SetPosition(Vector3(x * patchSize, 0, z * patchSize));
                instance.transform = vk::RayInstanceTransform(model);
                instances.push_back(std::move(instance));
                ++terrainCount;
            }
        std::erase_if(_rtTerrainPatches, [&](const auto& item) { return !used.contains(item.first); });
    }
    else
        _rtTerrainPatches.clear();
    _vk.SetRTCoverage(terrainCount, foliageCount);
    GfxMatrix transform{};
    ConvertMatrix(transform, camera->Transform());
    std::array<float, 16> viewToWorld{};
    std::copy_n(&transform._11, 16, viewToWorld.begin());
    _vk.SetRTAOScene(std::move(instances), viewToWorld);
}
} // namespace Poseidon
