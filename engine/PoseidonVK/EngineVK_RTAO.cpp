#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/VertexBufferVK.hpp>
#include <PoseidonVK/RayTransformVK.hpp>
#include <PoseidonVK/RaySurfaceVK.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/TerrainRayVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/World/Terrain/LandscapeShared.hpp>
#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <set>
#include <Poseidon/World/World.hpp>
#include <Poseidon/World/Entities/Infantry/Person.hpp>
#include <Poseidon/AI/AIUnit.hpp>
#include <Poseidon/World/Entities/Vehicles/Transport.hpp>
#include <chrono>
#include <cstring>

namespace Poseidon
{
namespace
{
constexpr size_t DynamicObjects = 32, DynamicPeople = 12, DynamicScan = 2048;
constexpr uint32_t DynamicVertices = 65536, DynamicObjectVertices = 8192;
double ElapsedRT(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}
} // namespace

RString EngineVK::SetRTDynamic(RString mode)
{
    const auto result = _vk.SetRTDynamic(static_cast<const char*>(mode));
    if (!_vk.RTDynamicEnabled())
        _rtDynamicCandidates.clear();
    return result.c_str();
}

void EngineVK::BeginRTDynamic()
{
    _rtDynamicCandidates.clear();
    _rtDynamicStats = {};
    _vk.BeginRTDynamic();
    if (!_vk.RTDynamicEnabled() || !GWorld || !GScene || !GScene->GetCamera())
        return;
    const auto start = std::chrono::steady_clock::now();
    const auto camera = GScene->GetCamera()->Position();
    const float radius = _vk.RTShadowsEnabled() || _vk.RTHitDataEnabled() ? 160.f : 80.f;
    std::set<uint64_t> seen;
    size_t scanned = 0;
    auto add = [&](Object* object)
    {
        if (!object || object->Static() || !object->GetShape() || !seen.insert(object->RenderIdentity()).second)
            return;
        const float distance2 = object->Position().Distance2(camera);
        if (distance2 > Square(radius + object->GetRadius()) ||
            object->GetObjSpecial() & (IsHidden | IsHiddenProxy | IsAlpha | IsTransparent | NoZBuf | NoZWrite))
            return;
        if ((!dynamic_cast<Transport*>(object) && !dynamic_cast<Person*>(object)) || object->IsDammageDestroyed())
        {
            ++_rtDynamicStats.unsupported;
            return;
        }
        if (const auto* person = dynamic_cast<Person*>(object);
            person && person->Brain() && person->Brain()->GetVehicleIn())
        {
            // Crew are rendered through seat proxies, not the person's world
            // transform. Do not invent an independently positioned body.
            ++_rtDynamicStats.unsupported;
            return;
        }
        _rtDynamicCandidates.push_back({object, object->RenderIdentity(), distance2, false});
    };
    // Existing simulation lists only. The cap also bounds CPU work in modded battles.
    for (int i = 0; i < GWorld->NVehicles() && scanned < DynamicScan; ++i, ++scanned)
        add(GWorld->GetVehicle(i));
    for (int i = 0; i < GWorld->NFastVehicles() && scanned < DynamicScan; ++i, ++scanned)
        add(GWorld->GetFastVehicle(i));
    if (size_t(GWorld->NVehicles() + GWorld->NFastVehicles()) > scanned)
        ++_rtDynamicStats.budget;
    std::sort(_rtDynamicCandidates.begin(), _rtDynamicCandidates.end(), [](const auto& a, const auto& b)
              { return a.distance2 != b.distance2 ? a.distance2 < b.distance2 : a.id < b.id; });
    size_t people = 0, count = 0;
    std::erase_if(_rtDynamicCandidates,
                  [&](const auto& candidate)
                  {
                      if (count >= DynamicObjects ||
                          (dynamic_cast<Person*>(candidate.object) && ++people > DynamicPeople))
                      {
                          ++_rtDynamicStats.budget;
                          return true;
                      }
                      ++count;
                      return false;
                  });
    _rtDynamicStats.selected = uint32_t(_rtDynamicCandidates.size());
    _rtDynamicStats.collectionMs += ElapsedRT(start);
}

void EngineVK::CaptureRayGeometry(Object& object, const Shape& shape, int level, const FrameBase& frame)
{
    if (!_vk.RTDynamicEnabled() || _rtDynamicCandidates.empty())
        return;
    auto found = std::find_if(_rtDynamicCandidates.begin(), _rtDynamicCandidates.end(),
                              [&](const auto& item) { return item.object == &object && !item.captured; });
    if (found == _rtDynamicCandidates.end() || !object.GetShape() || level < 0 ||
        level >= object.GetShape()->NLevels() || object.GetShape()->Resolution(level) >= 1000)
        return; // Never capture view-pilot, collision, memory or proxy geometry.
    found->captured = true;
    const auto start = std::chrono::steady_clock::now();
    try
    {
        if (shape.NVertex() <= 0 || shape.NNorm() != shape.NVertex() ||
            (shape.GetOrHints() & (ClipLandKeep | ClipLandOn)))
        {
            ++_rtDynamicStats.unsupported;
        }
        else if (shape.NVertex() > DynamicObjectVertices ||
                 _rtDynamicStats.vertices + shape.NVertex() > DynamicVertices || shape.NFaces() > 16384 ||
                 shape.NSections() > 256)
        {
            ++_rtDynamicStats.budget;
        }
        else
        {
            auto* rigid = !object.IsAnimated(level) && !shape.IsAnimated() && object.GetTotalDammage() == 0
                              ? dynamic_cast<VertexBufferVK*>(shape.GetVertexBuffer())
                              : nullptr;
            if (rigid && rigid->bufferDirty)
                rigid = nullptr;
            auto geometry = rigid ? vk::ShapeGeometry{} : vk::ExtractShapeGeometry(shape);
            vk::RTAOInstance instance;
            instance.dynamicId = found->id;
            instance.transform = vk::RayInstanceTransform(frame.Transform());
            instance.index16 = sizeof(VertexIndex) == 2;
            instance.dynamicGeometry = !rigid;
            if (rigid)
                instance.mesh = rigid->Buffers();
            for (int s = 0; s < shape.NSections(); ++s)
            {
                const auto& section = shape.GetSection(s);
                const int flags = section.properties.Special() | object.GetObjSpecial();
                if (flags & (IsAlpha | IsAlphaFog | IsHidden | IsHiddenProxy | IsShadow | NoZBuf | NoZWrite |
                             OnSurface | IsOnSurface | NoDropdown | IsLight | NoShadow | ShadowDisabled))
                    continue;
                auto* texture = section.properties.GetTexture();
                auto* tex = dynamic_cast<TextureVK*>(texture);
                const auto alpha = texture ? texture->GetAlphaClass() : AlphaStats::Opaque;
                if ((tex && tex->IsDynamic()) || alpha == AlphaStats::Blend)
                {
                    ++_rtDynamicStats.unsupported;
                    continue;
                }
                const auto range = rigid ? rigid->Sections()[s] : geometry.sections[s];
                if (range.begin == range.end)
                    continue;
                vk::RTAORange material{range.begin, range.end - range.begin};
                material.sampler = vk::ShapeSampler(render::SplitLegacy(flags)) | vk::RaySurfaceFlags(flags);
                if ((flags & IsTransparent) || alpha == AlphaStats::Cutout)
                {
                    if (!tex)
                        continue;
                    material.alpha = tex->Image(_vk);
                    material.cutoff = flags & IsTransparent ? 192.f / 255 : .5f;
                }
                if (tex && (_vk.RTReflectionsEnabled() || material.alpha))
                {
                    const auto image = tex->Image(_vk);
                    const auto& pixels = tex->Pixels();
                    _vk.PrepareRTAOAlpha(image, pixels.width, pixels.height, pixels.rgba.data());
                    if (_vk.RTReflectionsEnabled())
                        material.color = image;
                }
                instance.ranges.push_back(std::move(material));
            }
            _rtDynamicStats.proxies += object.GetProxyCount(level);
            if (!instance.ranges.empty())
            {
                if (_vk.AddRTDynamic(std::move(instance), geometry.vertices.data(),
                                     geometry.vertices.size() * sizeof(vk::ShapeVertex), geometry.indices.data(),
                                     geometry.indices.size() * sizeof(VertexIndex)))
                {
                    _rtDynamicStats.vertices += uint32_t(shape.NVertex());
                    ++_rtDynamicStats.captured;
                }
            }
            else
                ++_rtDynamicStats.unsupported;
        }
    }
    catch (const std::exception& error)
    {
        ++_rtDynamicStats.unsupported;
        LOG_WARN(Graphics, "Dynamic RT geometry excluded: {}", error.what());
    }
    _rtDynamicStats.collectionMs += ElapsedRT(start);
}

void EngineVK::FinishRTDynamic()
{
    if (!_vk.RTDynamicEnabled())
    {
        _rtDynamicCandidates.clear();
        return;
    }
    // Objects absent from the raster frustum still affect rays. Reuse Animate /
    // Deanimate exactly as the existing local-light caster collector does.
    for (auto& candidate : _rtDynamicCandidates)
    {
        if (candidate.captured)
            continue;
        auto* object = candidate.object;
        auto* lod = object->GetShape();
        if (!lod)
            continue;
        int level = GScene->LevelFromDistance2(lod, candidate.distance2, object->Scale(), object->Direction(),
                                               GScene->GetCamera()->Direction());
        if (level == LOD_INVISIBLE || level < 0 || level >= lod->NLevels() || lod->Resolution(level) >= 1000)
        {
            ++_rtDynamicStats.unsupported;
            continue;
        }
        Shape* shape = lod->LevelOpaque(level);
        if (!shape || shape->NVertex() > DynamicObjectVertices ||
            _rtDynamicStats.vertices + shape->NVertex() > DynamicVertices)
        {
            ++_rtDynamicStats.budget;
            continue;
        }
        const auto start = std::chrono::steady_clock::now();
        object->Animate(level);
        _rtDynamicStats.collectionMs += ElapsedRT(start);
        struct RestorePose
        {
            Object* object;
            int level;
            ~RestorePose() { object->Deanimate(level); }
        } restore{object, level};
        const auto captured = _rtDynamicStats.captured;
        CaptureRayGeometry(*object, *shape, level, *object);
        _rtDynamicStats.fallback += _rtDynamicStats.captured - captured;
    }
    _vk.SetRTDynamicStats(_rtDynamicStats);
    _rtDynamicCandidates.clear();
}

void EngineVK::CollectRTAOScene()
{
    if (!_vk.RaySceneEnabled() || !GScene || !GScene->GetCamera() || !GScene->GetLandscape())
        return;
    const auto* camera = GScene->GetCamera();
    auto* land = GScene->GetLandscape();
    const auto position = camera->Position();
    const bool hitData = _vk.RTHitDataEnabled();
    const bool shadows = _vk.RTShadowsEnabled() || hitData;
    const float radius = shadows ? 160.f : 80.f;
    const auto sunDirection = -GScene->MainLight()->Direction().Normalized();
    const float daylight = std::clamp(1.f - GScene->MainLight()->NightEffect(), 0.f, 1.f);
    _vk.SetRTSun({sunDirection.X(), sunDirection.Y(), sunDirection.Z(), sunDirection.Y() > .01f ? daylight : 0.f});
    if (hitData)
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
                                    1.f / std::max(1.f, GScene->GetFogMaxRange() - GScene->GetFogMinRange()), 0, 0},
                                   rgba(sun->Ambient()), rgba(sun->Diffuse())});
    }
    uint32_t foliageCount = 0, terrainCount = 0;
    // Existing spatial buckets, independent of visibility and the CSM toggle.
    // AO fades out at 60m; the extra 20m covers short rays and nearby bounds.
    int x0, x1, z0, z1;
    ObjRadiusRectangle(x0, x1, z0, z1, position, position, radius);
    std::vector<vk::RTAOInstance> instances;
    const auto fittedSerial=++_rtFittedSerial;
    std::set<const Object*> visited;
    const size_t sceneLimit = 1024 - (_vk.RTDynamicEnabled() ? DynamicObjects : 0);
    const size_t objectLimit = shadows ? 800 : _vk.RTAOEnabled() ? std::min(size_t(960), sceneLimit) : sceneLimit;
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
                if (lod->NLevels() == 0)
                    continue;
                Shape* shape = lod->LevelOpaque(0);
                // Validated stock walls can be CPU/terrain-fitted. Capture the
                // SAME Animate/Deanimate pose used by raster, including offscreen
                // walls, instead of substituting its un-fitted source geometry.
                bool fitted=false;
                if(shape && (shape->GetOrHints() & ClipLandMask)==ClipLandKeep && shape->NSections()>0)
                {
                    fitted=true;
                    for(int s=0; s<shape->NSections() && fitted; ++s)
                    {
                        vk::ShapeLighting material;
                        std::shared_ptr<vk::TextureImage> color,normal;
                        const auto* tex=dynamic_cast<TextureVK*>(shape->GetSection(s).properties.GetTexture());
                        fitted=tex && ApplyEnhancedMaterial(*shape,*tex,material,color,normal);
                    }
                }
                if(object->IsAnimated(0) && !fitted) continue;
                if (!shape || shape->IsAnimated() || shape->NVertex() == 0 ||
                    (!fitted && (shape->GetOrHints() & (ClipLandKeep | ClipLandOn))))
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
                    candidate.sampler = vk::ShapeSampler(render::SplitLegacy(flags)) |
                                        vk::RaySurfaceFlags(flags | object->GetObjSpecial());
                    if (hitData)
                        if (auto* tex = dynamic_cast<TextureVK*>(texture); tex && !tex->IsDynamic())
                        {
                            // RT water keeps its color contract. GI also uploads
                            // qualified v3 stock RGB, independently of receiver adoption.
                            if(_vk.RTReflectionsEnabled())
                            {
                                candidate.color = tex->Image(_vk);
                                const auto& pixels = tex->Pixels();
                                _vk.PrepareRTAOAlpha(candidate.color, pixels.width, pixels.height, pixels.rgba.data());
                            }
                            vk::ShapeLighting enhanced;
                            bool materialVertices=shape->NNorm()==shape->NVertex();
                            if (_vk.RTGIEnabled() && vk::StockPBRIdentity(tex->MaterialKey()))
                                for(Offset at=section.beg; at<section.end && materialVertices; shape->NextFace(at))
                                {
                                    const auto& face=shape->Face(at);
                                    for(int v=0;v<face.N();++v)
                                        if(shape->Clip(face.GetVertex(v))&ClipUserMask) { materialVertices=false; break; }
                                }
                            if (_vk.RTGIEnabled() && alpha == AlphaStats::Opaque &&
                                materialVertices && (candidate.sampler & vk::RayDiffuse) && vk::StockPBRIdentity(tex->MaterialKey()))
                            {
                                candidate.color=tex->Image(_vk);
                                const auto& pixels=tex->Pixels();
                                _vk.PrepareRTAOAlpha(candidate.color,pixels.width,pixels.height,pixels.rgba.data());
                                candidate.enhanced={vk::PBRMaterial(tex->MaterialKey(),0),0,.04f,
                                    float(vk::StockPBRIdentity(tex->MaterialKey()))};
                            }
                            if (!((flags | object->GetObjSpecial()) & (DisableSun | IsColored | IsLight | IsAlphaFog)) &&
                                ApplyEnhancedMaterial(*shape, *tex, enhanced, candidate.color, candidate.normalRoughness))
                            {
                                candidate.enhanced = {enhanced.pbr[0], enhanced.pbr[2], enhanced.pbr[1], enhanced.pbr[3]};
                                candidate.sampler |= vk::RayDiffuse;
                                if (enhanced.materialAmbient[3] > .5f) candidate.sampler |= vk::RayNormalMap;
                                if (enhanced.materialSun[3] > .5f) candidate.sampler |= vk::RayRoughnessMap;
                            }
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
                        candidate.cutoff = flags & IsTransparent ? 192.f / 255 : .5f;
                    }
                    (extended ? shadowOnly : instance.ranges).push_back(std::move(candidate));
                }
                if (instance.ranges.empty() && shadowOnly.empty())
                    continue;
                std::vector<vk::SectionRange> fittedSections;
                if(fitted)
                {
                    auto& cached=_rtFittedMeshes[object->RenderIdentity()];
                    cached.seen=fittedSerial;
                    object->Animate(0);
                    struct Restore { Object* object; ~Restore(){object->Deanimate(0);} } restore{object};
                    auto geometry=vk::ExtractShapeGeometry(*shape);
                    const bool same=cached.geometry.vertices.size()==geometry.vertices.size() &&
                        cached.geometry.indices==geometry.indices &&
                        !std::memcmp(cached.geometry.vertices.data(),geometry.vertices.data(),geometry.vertices.size()*sizeof(vk::ShapeVertex));
                    if(!same || !cached.mesh)
                    {
                        cached.mesh=_vk.UploadMesh(geometry.vertices.data(),geometry.vertices.size()*sizeof(vk::ShapeVertex),
                            geometry.indices.data(),geometry.indices.size()*sizeof(VertexIndex));
                        cached.geometry=std::move(geometry);
                    }
                    instance.mesh=cached.mesh;
                    fittedSections=cached.geometry.sections;
                }
                // Do not force a raster buffer for excluded foliage/effect models.
                if (!fitted && !shape->GetVertexBuffer())
                    shape->ConvertToVBuffer(VBStatic);
                auto* buffer = dynamic_cast<VertexBufferVK*>(shape->GetVertexBuffer());
                if (!fitted && !buffer)
                    continue;
                if(!fitted) { buffer->Update(*shape, false); instance.mesh = buffer->Buffers(); }
                for (auto* ranges : {&instance.ranges, &shadowOnly})
                {
                    for (auto& range : *ranges)
                    {
                        const auto section = (fitted?fittedSections:buffer->Sections()).at(range.first);
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
    std::erase_if(_rtFittedMeshes,[&](const auto& entry){return entry.second.seen!=fittedSerial;});
    // Actual subdivided terrain samples and the same 01--10 cell diagonal as
    // Landscape::SurfaceY. Only local patches; height changes invalidate a patch.
    if (shadows || _vk.RTAOEnabled())
    {
        const float grid = land->GetTerrainGrid();
        if (_rtLandscape != land || _rtTerrainGrid != grid)
        {
            _rtTerrainPatches.clear();
            _rtLandscape = land;
            _rtTerrainGrid = grid;
        }
        constexpr int cells = 16;
        const float patchSize = cells * grid;
        std::set<std::pair<int, int>> used;
        for (int z = int(std::floor((position.Z() - radius) / patchSize));
             z <= int(std::floor((position.Z() + radius) / patchSize)); ++z)
            for (int x = int(std::floor((position.X() - radius) / patchSize));
                 x <= int(std::floor((position.X() + radius) / patchSize)) && instances.size() < sceneLimit; ++x)
            {
                if (x < 0 || z < 0 || (x + 1) * cells >= land->GetTerrainRange() ||
                    (z + 1) * cells >= land->GetTerrainRange())
                    continue;
                const auto key = std::make_pair(x, z);
                used.insert(key);
                std::vector<float> heights;
                // Include the normal stencil in the cache key as well as positions.
                for (int j = -1; j <= cells+1; ++j)
                    for (int i = -1; i <= cells+1; ++i)
                        heights.push_back(land->SurfaceY((x * cells + i) * grid, (z * cells + j) * grid));
                std::vector<int> textureIds;
                for (int j=0; j<cells; ++j) for(int i=0; i<cells; ++i)
                    textureIds.push_back(land->GetTexture(int((z*cells+j)*grid/land->GetLandGrid()),
                                                        int((x*cells+i)*grid/land->GetLandGrid())));
                auto& patch = _rtTerrainPatches[key];
                if (!patch.mesh || patch.heights != heights || patch.textured != hitData ||
                    patch.textures != textureIds || patch.gi != _vk.RTGIEnabled())
                {
                    std::vector<std::array<float, 8>> vertices;
                    std::vector<uint32_t> indices;
                    std::map<std::pair<int,bool>, std::vector<uint32_t>> groups;
                    auto height=[&](int i,int j) { return heights[(j+1)*(cells+3)+i+1]; };
                    for (int j = 0; j < cells; ++j)
                        for (int i = 0; i < cells; ++i)
                        {
                            const int id=textureIds[j*cells+i];
                            auto* texture=dynamic_cast<TextureVK*>(land->GetTexture(id));
                            const float wx=(x*cells+i)*grid/land->GetLandGrid(), wz=(z*cells+j)*grid/land->GetLandGrid();
                            const int lx=int(std::floor(wx)),lz=int(std::floor(wz));
                            std::array<float,4> u{},v{};
                            land->GetRandomColor(lx,lz,u[0],v[0]); land->GetRandomColor(lx+1,lz,u[1],v[1]);
                            land->GetRandomColor(lx,lz+1,u[2],v[2]); land->GetRandomColor(lx+1,lz+1,u[3],v[3]);
                            const bool clamp=id>0 && (land->ClampFlags(id)&ClampU);
                            const uint32_t a=uint32_t(vertices.size());
                            bool dry=id>0;
                            for(int dz=0;dz<2;++dz) for(int dx=0;dx<2;++dx)
                            {
                                const int vi=i+dx,vj=j+dz;
                                const auto normal=vk::TerrainRayNormal(height(vi-1,vj),height(vi+1,vj),
                                    height(vi,vj-1),height(vi,vj+1),grid);
                                const auto uv=vk::TerrainRayUV(wx+dx*grid/land->GetLandGrid(),
                                    wz+dz*grid/land->GetLandGrid(),float(lx),float(lz),clamp,u,v);
                                vertices.push_back({vi*grid,height(vi,vj),vj*grid,normal[0],normal[1],normal[2],
                                    texture?texture->UToPhysical(uv[0]):0,texture?texture->VToPhysical(uv[1]):0});
                                // Same potential-water bound as the raster terrain.
                                // Stable across waves/tides; no per-frame BLAS churn.
                                dry=dry && height(vi,vj)>maxTide+maxWave;
                            }
                            auto& group=groups[{id,dry}];
                            group.insert(group.end(),{a,a+1,a+2,a+1,a+3,a+2});
                        }
                    patch.ranges.clear();
                    patch.textured = hitData;
                    patch.gi = _vk.RTGIEnabled();
                    patch.textures = std::move(textureIds);
                    for (auto& [identity, group] : groups)
                    {
                        vk::RTAORange range{uint32_t(indices.size()), uint32_t(group.size())};
                        auto* texture=dynamic_cast<TextureVK*>(land->GetTexture(identity.first));
                        range.sampler = vk::RayDiffuse | vk::RayVertexNormals;
                        if(identity.first>0) range.sampler|=vk::ShapeSampler(render::SplitLegacy(land->ClampFlags(identity.first)));
                        if (hitData && texture && !texture->IsDynamic())
                        {
                            range.color = texture->Image(_vk);
                            const auto& pixels = texture->Pixels();
                            _vk.PrepareRTAOAlpha(range.color, pixels.width, pixels.height, pixels.rgba.data());
                            // Landscape-owned opaque dry ground, not arbitrary object
                            // atlases, water, submerged seabed or foliage impostors.
                            if (_vk.RTGIEnabled() && identity.second && texture->GetAlphaClass()==AlphaStats::Opaque)
                            {
                                range.enhanced={1,0,.04f,1000};
                                LOG_INFO(Graphics,"RTGI terrain source {} {}x{} patch={},{} triangles={} clamp={} dry=yes",
                                    texture->MaterialKey(),pixels.width,pixels.height,x,z,group.size()/3,range.sampler&3);
                            }
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
                // Bit 2 is AO terrain; foliage's shadow-only bit remains separate.
                instance.mask = 2 | (_vk.RTAOEnabled() ? 4 : 0);
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
