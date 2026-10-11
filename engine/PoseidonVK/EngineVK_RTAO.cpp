#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/VertexBufferVK.hpp>
#include <PoseidonVK/ShapeTransformVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <set>

namespace Poseidon
{
void EngineVK::CollectRTAOScene()
{
    if (!_vk.RTAOEnabled() || !GScene || !GScene->GetCamera() || !GScene->GetLandscape())
        return;
    const auto* camera = GScene->GetCamera();
    auto* land = GScene->GetLandscape();
    const auto position = camera->Position();
    // Existing spatial buckets, independent of visibility and the CSM toggle.
    // AO fades out at 60m; the extra 20m covers short rays and nearby bounds.
    int x0, x1, z0, z1;
    ObjRadiusRectangle(x0, x1, z0, z1, position, position, 80.f);
    std::vector<vk::RTAOInstance> instances;
    std::set<const Object*> visited;
    for (int z = z0; z <= z1 && instances.size() < 1024; ++z)
        for (int x = x0; x <= x1 && instances.size() < 1024; ++x)
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
                if (object->Position().Distance2(position) > Square(80.f + object->GetRadius()))
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
                    if (alpha == AlphaStats::Blend)
                        continue;
                    vk::RTAORange candidate{uint32_t(s), 0};
                    if ((flags & IsTransparent) || alpha == AlphaStats::Cutout)
                    {
                        // Building walls often use alpha for real windows. Test those
                        // holes during traversal. Foliage and uncertain materials stay out.
                        auto* vkTexture = dynamic_cast<TextureVK*>(texture);
                        if (object->GetDestructType() != DestructBuilding || !vkTexture || vkTexture->IsDynamic())
                            continue;
                        candidate.alpha = vkTexture->Image(_vk);
                        const auto& pixels = vkTexture->Pixels();
                        _vk.PrepareRTAOAlpha(candidate.alpha, pixels.width, pixels.height, pixels.rgba.data());
                        candidate.sampler = vk::ShapeSampler(render::SplitLegacy(flags));
                        candidate.cutoff = flags & IsTransparent ? 192.f / 255 : .5f;
                    }
                    instance.ranges.push_back(std::move(candidate));
                }
                if (instance.ranges.empty())
                    continue;
                // Do not force a raster buffer for excluded foliage/effect models.
                if (!shape->GetVertexBuffer())
                    shape->ConvertToVBuffer(VBStatic);
                auto* buffer = dynamic_cast<VertexBufferVK*>(shape->GetVertexBuffer());
                if (!buffer)
                    continue;
                instance.mesh = buffer->Buffers();
                for (auto& range : instance.ranges)
                {
                    const auto section = buffer->Sections().at(range.first);
                    range.first = section.begin;
                    range.count = section.end - section.begin;
                }
                std::erase_if(instance.ranges, [](const auto& range) { return range.count == 0; });
                if (instance.ranges.empty())
                    continue;
                instance.transform = vk::RayInstanceTransform(object->Transform());
                instances.push_back(std::move(instance));
                if (instances.size() == 1024)
                    break;
            }
        }
    GfxMatrix transform{};
    ConvertMatrix(transform, camera->Transform());
    std::array<float, 16> viewToWorld{};
    std::copy_n(&transform._11, 16, viewToWorld.begin());
    _vk.SetRTAOScene(std::move(instances), viewToWorld);
}
} // namespace Poseidon
