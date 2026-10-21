#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <PoseidonVK/ShapeLightingVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/TerrainRayVK.hpp>
#include <Poseidon/World/Terrain/LandscapeShared.hpp>

namespace Poseidon
{
void EngineVK::CaptureTerrainMaterial(TLVertexTable& table, const VertexTable& mesh,
                                      const PackedColor* palette, bool water)
{
    table.terrainMaterial.clear();
    if (!_vk.RTGIEnabled() || !_vk.EnhancedMaterialsEnabled() || !_shadowWorld || water || !palette ||
        mesh.NNorm()!=mesh.NVertex()) return;
    table.terrainMaterial.resize(table.NVertex());
    for (int i=0;i<table.NVertex();++i)
    {
        const auto& n=mesh.Norm(i);
        const Color tint=Color(palette[(table.Clip(i)&ClipUserMask)/ClipUserStep])*2;
        table.terrainMaterial[i]=vk::TerrainReceiverMaterial(mesh.Pos(i).Y(),maxTide+maxWave,
            {-n.X(),-n.Y(),-n.Z()},{tint.R(),tint.G(),tint.B()});
    }
}
void EngineVK::PrepareMesh(const render::LegacySpec&)
{
    _softwarePBRLighting.clear();
    _softwareSourceShape = nullptr;
    // CPU transformation, lighting and clipping are performed by FaceArray.
    if (_softwareMesh || _activeShape)
        throw std::logic_error("Vulkan software mesh preparation requires an idle open frame");
}
void EngineVK::CaptureSoftwareMaterial(TLVertexTable& dst, const TLMaterial& mat, const Matrix4& worldToModel,
                                       const LightList& lights, const VertexTable& mesh, int beg, int end)
{
    if (!_vk.PBREnabled() || !_shadowWorld || _waterSurface) return;
    const auto* shape = dynamic_cast<const Shape*>(&mesh);
    if (!shape) return; // Colorized terrain has its own lighting; never invent material inputs.
    bool candidate = false;
    for (int s = 0; s < shape->NSections(); ++s)
        if (const auto* texture = dynamic_cast<TextureVK*>(shape->GetSection(s).properties.GetTexture());
            texture && (vk::PBRMaterial(texture->MaterialKey(), mat.specularPower, _vk.EnhancedMaterialsEnabled()) > 0 ||
                (_vk.AutomaticMaterialsEnabled() && (texture->AutomaticMaterial(shape->GetSection(s).surfMat) ||
                    texture->RegionalMaterial(vk::OriginalRegionCandidate(*shape,texture->MaterialKey()))))))
        {
            candidate = true;
            break;
        }
    if (!candidate) return;
    _softwareSourceShape = shape;
    dst.pbrNormal.resize(dst.NVertex());
    // Explicit inside/half-lit/shining and colored modes retain their authored
    // response. No reconstruction from packed lighting or guessed face normals.
    if ((mat.specFlags & (DisableSun | IsShadow | IsLight | IsColored | IsAlphaFog | NoDropdown | NoZBuf)) ||
        mat.diffuse.R() < .99f || mat.diffuse.G() < .99f || mat.diffuse.B() < .99f ||
        mesh.NNorm() != mesh.NVertex())
    {
        for (int i = beg; i < end; ++i) dst.pbrNormal[i] = {};
        return;
    }
    vk::ShapeLighting lighting;
    vk::ShapeMaterial(lighting, mat, *GScene->MainLight(), true);
    GfxMatrix materialTransform{};
    ConvertMatrix(materialTransform, worldToModel.InverseGeneral());
    std::copy_n(&materialTransform._11, 16, lighting.materialToWorld.begin());
    int count = 0;
    const float night = GScene->MainLight()->NightEffect();
    if (night >= .01f)
        for (int i = 0; i < lights.Size(); ++i)
        {
            if (!lights[i]) continue;
            LightDescription light;
            lights[i]->GetDescription(light);
            // Preserve the complete local-light set or leave the range legacy.
            if ((light.type != LTPoint && light.type != LTSpotLight) || count == int(lighting.localLights.size()))
            {
                for (int v = beg; v < end; ++v) dst.pbrNormal[v] = {};
                return;
            }
            vk::ShapeLight(lighting.localLights[count], light, mat, GScene->GetCamera()->Position(), night);
            for (int slot = 0; slot < _localShadowCount; ++slot)
                if (_vk.LocalShadowsEnabled() && lights[i].GetRef() == static_cast<Light*>(_localShadowLights[slot]))
                    lighting.localLights[count].diffuse[3] = float(slot + 1);
            ++count;
        }
    lighting.localCount[0] = float(count);
    _softwarePBRLighting.push_back(lighting);
    const float slot = float(_softwarePBRLighting.size());
    // Inverse transpose(model->world) = transpose(world->model). Stock engine
    // normals face inward; use the same sign as ExtractShapeGeometry.
    for (int i = beg; i < end; ++i)
    {
        if ((dst.Clip(i) & ClipUserMask) ||
            (_vk.AutomaticMaterialsEnabled() && !vk::OriginalOrdinaryVertex(mesh.Clip(i))))
        {
            dst.pbrNormal[i] = {};
            continue;
        }
        const auto& n = mesh.Norm(i);
        Vector3 normal = vk::SoftwarePBRNormal(worldToModel, n);
        if (std::isfinite(normal.SquareSize()) && normal.SquareSize() > 1e-12f)
        {
            normal.Normalize();
            dst.pbrNormal[i] = {normal.X(), normal.Y(), normal.Z(), slot};
        }
        else dst.pbrNormal[i] = {};
    }
}
void EngineVK::BeginMesh(TLVertexTable& mesh, const render::LegacySpec&)
{
    if (_temporalTerrain && !_waterSurface)
        CaptureTemporalTerrain(mesh, false);
    if (_softwareMesh || _activeShape)
        throw std::logic_error("Vulkan software BeginMesh requires an idle open frame");
    _softwareRemap.assign(mesh.NVertex(), UINT32_MAX);
    _softwareMesh = &mesh;
}
void EngineVK::PrepareTriangle(const MipInfo& mip, int flags)
{
    const int allowed = (_terrainSurface ? DetailTexture : 0) | (_waterSurface ? DetailTexture | SpecularTexture : 0) |
                        NoZBuf | NoZWrite | IsAlpha | IsTransparent | ClampU | ClampV | NoClamp | PointSampling |
                        BestMipmap | FogDisabled | DisableSun | IsColored | NoShadow | IsAlphaOrdered | NoDropdown |
                        IsAlphaFog | OnSurface | IsOnSurface | ShadowDisabled | NoTexMerger | IsAnimated | ZBiasMask |
                        SpecLighting | IsLight | IsShadow; // Lighting/fades are already done by TLVertexTable.
    if (flags & ~allowed)
    {
        LOG_ERROR(Graphics, "Vulkan software section has unsupported flags 0x{:x}", flags & ~allowed);
        Unsupported("software section flags outside diffuse sampling");
    }
    if (!_softwareMesh || !mip.IsOK())
        throw std::logic_error("Vulkan software section requires an active mesh and valid mip");
    _softwareMip = mip;
    _softwareFlags = flags;
}
void EngineVK::DrawPolygon(const VertexIndex* vertices, int n)
{
    if (!vertices || n < 3)
        throw std::invalid_argument("Vulkan software polygon requires at least three vertices");
    std::vector<uint32_t> indices;
    for (int i = 2; i < n; ++i)
        indices.insert(indices.end(), {uint32_t(vertices[0]), uint32_t(vertices[i - 1]), uint32_t(vertices[i])});
    SubmitSoftware(indices);
}
void EngineVK::DrawSection(const FaceArray& faces, Offset begin, Offset end)
{
    std::vector<uint32_t> indices;
    for (Offset i = begin; i < end; faces.Next(i))
    {
        const auto& face = faces[i];
        for (int v = 2; v < face.N(); ++v)
            indices.insert(indices.end(),
                           {uint32_t(face.GetVertex(0)), uint32_t(face.GetVertex(v - 1)), uint32_t(face.GetVertex(v))});
    }
    SubmitSoftware(indices,_vk.AutomaticMaterialsEnabled()?faces.FindSection(begin,end):nullptr);
}
void EngineVK::SubmitSoftware(const std::vector<uint32_t>& indices, const ShapeSection* section)
{
    auto* texture = dynamic_cast<TextureVK*>(_softwareMip._texture);
    const auto state = vk::SoftwareAlpha(_softwareFlags, texture ? texture->GetAlphaClass() : AlphaStats::Opaque);
    const auto source=texture && _vk.AutomaticMaterialsEnabled() && section &&
        section->properties.GetTexture()==texture ?
        vk::OriginalSectionSource(texture->MaterialKey(),section) : vk::QualifiedMaterial{};
    const auto regional=texture && _vk.AutomaticMaterialsEnabled() && _softwareSourceShape && section &&
        section->properties.GetTexture()==texture ? texture->RegionalMaterial(
            vk::OriginalRegionCandidate(*_softwareSourceShape,texture->MaterialKey())) : vk::QualifiedMaterial{};
    const auto candidate=source?source:regional;
    if (!_softwareMesh || !_vk.PBREnabled() || !_shadowWorld || _waterSurface || !texture ||
        texture->IsTransparent() || state.blend || !state.depthWrite || !state.depthTest ||
        (_softwareFlags & (DisableSun | IsShadow | IsLight | IsColored | IsAlphaFog | NoDropdown | SpecularTexture)) ||
        (vk::PBRMaterial(texture->MaterialKey(), 0, _vk.EnhancedMaterialsEnabled()) == 0 &&
         !(_vk.AutomaticMaterialsEnabled() && candidate)) || _softwareMesh->pbrNormal.empty() ||
        (_vk.AutomaticMaterialsEnabled() && vk::QualifyOriginalMaterial(candidate,vk::MaterialRejection::None,
            _softwareFlags,!texture->IsTransparent() && state.cutoff==0,!texture->IsDynamic())!=vk::MaterialRejection::None))
    {
        SubmitSoftwareLit(indices, nullptr);
        return;
    }
    // A texture section can span multiple vertex lighting modes. Split only at
    // actual captured-material boundaries, preserving face order and fallback.
    const auto material = [&](size_t i) -> std::pair<int,unsigned> {
        int slot = 0;
        for (size_t k = 0; k < 3; ++k)
        {
            const auto index = indices[i+k];
            if (index >= _softwareMesh->pbrNormal.size()) return {};
            if (_vk.AutomaticMaterialsEnabled())
            {
                const auto& vertex=_softwareMesh->GetVertex(index);
                if (!std::isfinite(vertex.t0.u) || !std::isfinite(vertex.t0.v)) return {};
            }
            const int next = int(_softwareMesh->pbrNormal[index][3]);
            if (next <= 0 || next > int(_softwarePBRLighting.size()) || (k && slot != next)) return {};
            slot = next;
        }
        if (_vk.AutomaticMaterialsEnabled() && slot)
        {
            const auto& lighting=_softwarePBRLighting[slot-1];
            if (lighting.rtSun[1]!=0 || lighting.rtSun[2]!=0 || lighting.rtSun[3]!=0 ||
                lighting.emissive[0]!=0 || lighting.emissive[1]!=0 || lighting.emissive[2]!=0) return {};
        }
        if (!source && regional)
        {
            std::array<VertexIndex,3> original;
            for (size_t k=0;k<3;++k)
            {
                const auto index=indices[i+k];
                // Do not turn clipping-generated subtriangles of a mixed face
                // into original wall regions. Original vertices/UVs must agree.
                if (index>=unsigned(_softwareSourceShape->NVertex())) return {};
                original[k]=VertexIndex(index);
                const auto& uv=_softwareSourceShape->UV(original[k]);
                const auto& vertex=_softwareMesh->GetVertex(index);
                if (uv.u!=vertex.t0.u || uv.v!=vertex.t0.v) return {};
            }
            const auto identity=vk::OriginalTriangleRegion(*_softwareSourceShape,regional.identity,original,_softwareFlags);
            if (!vk::SameOriginalRegionSource(regional.identity,identity) || !texture->RegionalMaterial(identity)) return {};
            return {slot,identity};
        }
        return {slot,candidate.identity};
    };
    for (size_t begin = 0; begin < indices.size();)
    {
        const auto [slot,identity] = material(begin);
        size_t end = begin + 3;
        while (end < indices.size() && material(end) == std::pair{slot,identity}) end += 3;
        const auto* lighting = slot ? &_softwarePBRLighting[slot-1] : nullptr;
        const auto adopted=source?source:
            (vk::SameOriginalRegionSource(regional.identity,identity)?texture->RegionalMaterial(identity):vk::QualifiedMaterial{});
        if (begin == 0 && end == indices.size()) SubmitSoftwareLit(indices, lighting, adopted);
        else
        {
            std::vector<uint32_t> part(indices.begin()+begin, indices.begin()+end);
            SubmitSoftwareLit(part, lighting, adopted);
        }
        begin = end;
    }
}
void EngineVK::SubmitSoftwareLit(const std::vector<uint32_t>& indices, const vk::ShapeLighting* material,
                                vk::QualifiedMaterial source)
{
    if (!_softwareMesh)
        throw std::logic_error("Vulkan software drawing requires BeginMesh");
    if (!_vk.FrameOpen())
        return; // A minimized drawable has no acquired frame; retain the engine mesh protocol.
    if (indices.empty())
        return;
    vk::CompactSoftwareIndices(indices,_softwareRemap,_softwareSources,_softwareIndices);
    _softwareVertices.resize(_softwareSources.size());
    size_t packedIndex=0;
    for (auto index : _softwareSources)
    {
        auto& packed=_softwareVertices[packedIndex++];
        // Clipping can leave unused original vertices without valid rhw.
        // Convert only the surviving face stream, as GL33's indexed queues do.
        const auto& source = _softwareMesh->GetVertex(index);
        Vertex2DAbs vertex;
        vertex.x = source.pos.X();
        vertex.y = source.pos.Y();
        vertex.z = source.pos.Z();
        vertex.w = source.rhw;
        vertex.u = source.t0.u;
        vertex.v = source.t0.v;
        vertex.color = source.color;
        packed = vk::ScreenGeometry(vertex, _width, _height);
        if (index < _softwareMesh->hdrColor.size() && _softwareMesh->hdrColor[index][3] != 0)
            std::copy_n(_softwareMesh->hdrColor[index].begin(), 3, packed.color);
        if (index < _softwareMesh->previousClip.size())
            std::copy(_softwareMesh->previousClip[index].begin(), _softwareMesh->previousClip[index].end(),
                      packed.previousClip);
        if (index < _softwareMesh->directSun.size())
            std::copy_n(_softwareMesh->directSun[index].begin(), 4, packed.directSun);
        if (index < _softwareMesh->directLocal.size())
            std::copy_n(_softwareMesh->directLocal[index].begin(), 6, packed.directLocal);
        std::fill_n(packed.aoAmbient, 4, 0.f);
        if (index < _softwareMesh->aoAmbient.size())
            std::copy_n(_softwareMesh->aoAmbient[index].begin(), 4, packed.aoAmbient);
        if (material)
        {
            std::copy_n(_softwareMesh->pbrNormal[index].begin(), 3, packed.pbrNormal);
            packed.aoAmbient[3] = 1.f;
        }
        if (index < _softwareMesh->terrainMaterial.size())
        {
            const auto& terrain=_softwareMesh->terrainMaterial[index];
            std::copy_n(terrain.begin(),3,packed.pbrNormal);
            std::copy_n(terrain.begin()+4,4,packed.terrainTint);
        }
        // TL already contains the scene Fog8 visibility in specular.a. Alpha
        // fog is already in color.a; it must not also get an RGB fog mix.
        packed.fog =
            (_softwareFlags & (FogDisabled | NoDropdown | IsAlphaFog | IsLight)) ? 1.f : float(source.specular >> 24) / 255;
    }
    auto mesh = _vk.UploadTransientMesh(_softwareVertices.data(), _softwareVertices.size() * sizeof(vk::ScreenVertex),
                                        _softwareIndices.data(), _softwareIndices.size() * sizeof(uint32_t));
    std::shared_ptr<vk::TextureImage> image;
    auto alpha = AlphaStats::Opaque;
    if (_softwareMip._texture)
    {
        auto* texture = dynamic_cast<TextureVK*>(_softwareMip._texture);
        if (!texture)
            throw std::logic_error("Vulkan software section received a foreign texture");
        image = texture->Image(_vk);
        alpha = texture->GetAlphaClass();
    }
    const bool shadow = (_softwareFlags & IsShadow) != 0;
    const auto state = vk::SoftwareAlpha(_softwareFlags, alpha);
    const bool blend = shadow ? alpha == AlphaStats::Blend || (_softwareFlags & (IsAlpha | IsAlphaFog | IsLight)) != 0
                              : state.blend;
    const float cutoff = shadow ? std::max(1, (GetShadowFactor() * 7) >> 4) / 255.f :
                                 state.cutoff;
    vk::ShapeLighting receiver = material ? *material : vk::ShapeLighting{};
    if (material)
    {
        auto* texture = static_cast<TextureVK*>(_softwareMip._texture);
        receiver.pbr = {vk::PBRMaterial(texture->MaterialKey(), int(receiver.specular[3]), _vk.EnhancedMaterialsEnabled()), .04f, 0, 0};
        if (_vk.AutomaticMaterialsEnabled() && source.identity==vk::SourceDiffuseIdentity)
            ApplySourceDiffuse(receiver);
        ApplyRegionalPBR(source,receiver);
        _vk.RecordPBRDraw(true);
    }
    receiver.aoReceiver[0] = _vk.RTAOEnabled() && _shadowWorld && !_waterSurface &&
        (!_softwareMesh->aoAmbient.empty() || material) && !blend &&
        !(_softwareFlags & (NoZBuf | NoZWrite | IsLight | DisableSun | NoDropdown | IsShadow | IsAlphaFog)) ? 1.f : 0.f;
    receiver.rtWater[3] = _vk.RTGIEnabled() && _shadowWorld && !_waterSurface && image &&
        alpha == AlphaStats::Opaque && !blend && cutoff == 0 &&
        !(_softwareFlags & (NoZBuf | NoZWrite | IsColored | IsLight | DisableSun | NoDropdown |
                            IsShadow | IsAlphaFog | SpecularTexture)) ? 1.f : 0.f;
    receiver.wetSurface = WetParameters(_softwareMip._texture, _softwareFlags);
    if (receiver.wetSurface[0] > 0 && GScene->MainLight())
    {
        const auto direction = GScene->MainLight()->Direction();
        receiver.sunDirection = {direction.X(), direction.Y(), direction.Z(), 0};
    }
    std::shared_ptr<vk::TextureImage> waterBump;
    float secondaryMode=0;
    if (_terrainSurface && (_softwareFlags&DetailTexture))
    {
        if (auto* detail=dynamic_cast<TextureVK*>(_textures.Load(Remaster >> "CfgDetailTextures" >> "detail").GetRef()))
            waterBump=detail->Image(_vk);
        secondaryMode=waterBump ? 1.f : 0.f;
    }
    // This path retains colorized CPU raster lighting, not the object PBR BRDF.
    // Every corner must have original dry-ground inputs and both captured terms.
    const bool terrain = _vk.EnhancedMaterialsEnabled() && receiver.rtWater[3]==1 && !material &&
        !_softwareMesh->terrainMaterial.empty() &&
        std::all_of(indices.begin(),indices.end(),[&](uint32_t i) {
            return i<_softwareMesh->terrainMaterial.size() && _softwareMesh->terrainMaterial[i][3]>.99f &&
                i<_softwareMesh->aoAmbient.size() && _softwareMesh->aoAmbient[i][3]>.99f &&
                i<_softwareMesh->directSun.size() && _softwareMesh->directSun[i][3]>.99f;
        });
    if (terrain)
    {
        const auto* texture=static_cast<TextureVK*>(_softwareMip._texture);
        if (!texture->IsDynamic())
        {
            receiver.rtWater[3]=2;
            receiver.pbr[3]=1000; // shared stock terrain identity; roughness 0 keeps legacy raster
            receiver.pbrDiffuse[3]=8; // spare diagnostic family: original CPU terrain
        }
    }
    if (material && _softwareSourceShape)
        if (const auto* texture = dynamic_cast<TextureVK*>(_softwareMip._texture))
        {
            ApplyStockPBR(*texture, receiver);
            ApplyEnhancedMaterial(*_softwareSourceShape, *texture, receiver, image, waterBump);
            // Material capture supplies independent ambient/direct inputs; no
            // subtraction from packed CPU-lit color is used for qualification.
            if (!_vk.AutomaticMaterialsEnabled() || vk::OriginalMaterialReception(vk::MaterialRejection::None,
                    material->ambient[3]>0,material->pbrDiffuse[0]>=.99f && material->pbrDiffuse[1]>=.99f &&
                    material->pbrDiffuse[2]>=.99f)) vk::QualifyGIReceiver(receiver, true);
        }
    receiver.rainRipples = _waterSurface ? RainRippleParameters() : std::array<float, 4>{};
    if (_waterSurface && (_vk.WaterSurfaceEnabled() || receiver.rainRipples[0] > 0))
    {
        try
        {
            auto texture = _textures.Load(Remaster >> "CfgDetailTextures" >> "specular");
            if (auto* bump = dynamic_cast<TextureVK*>(texture.GetRef()))
                waterBump = bump->Image(_vk);
            receiver.rtWater = {waterBump ? (_vk.RTReflectionsEnabled() ? 2.f : 1.f) : 0.f, 0, 0, 0}; // shoreline UVs already animate in Landscape::DrawMesh
            secondaryMode=waterBump ? 1.f : 0.f; // original scalar specular_dx alpha at 32x UV, not XYZ normals
            receiver.temporal[3] = 2;                            // reactive water, never stationary terrain history
        }
        catch (const std::exception& error)
        {
            if (_vk.RTReflectionsEnabled()) _vk.FailRTReflections(error.what());
            _vk.FailWaterRefraction(error.what());
        }
    }
    if (!receiver.rtWater[0])
        receiver.temporal[3] = _temporalTerrain ? -1.f : 0.f;
    receiver.fogColor = {_fogColor.R(), _fogColor.G(), _fogColor.B(), 1};
    receiver.eyeCoef = _vk.EyeCoef();
    const bool atmosphere = _shadowWorld && _vk.AtmosphereEnabled();
    const bool localShadows = _vk.LocalShadowsEnabled() && !_softwareMesh->directLocal.empty();
    if (_shadowWorld &&
        (_shadowTuning.enabled || atmosphere || _vk.SeparateSunlight() || receiver.rtWater[0] || localShadows || receiver.wetSurface[0] || material || terrain) &&
        !(_softwareFlags & (DisableSun | NoZBuf | NoDropdown | IsLight | IsShadow | IsAlphaFog)) && GScene &&
        GScene->GetCamera())
    {
        const auto* camera = GScene->GetCamera();
        Matrix4 rotation = camera->Transform();
        rotation.SetPosition(VZero);
        vk::ShapeWorld(receiver, rotation);
        const auto& projection = camera->ProjectionNormal();
        receiver.shadowReceiver = {_shadowTuning.enabled ? 1.f : 0.f, projection(0, 0), projection(1, 1), 0};
        receiver.atmosphereSun[3] = atmosphere ? 1.f : 0.f;
        receiver.rtSun[0] = (_vk.RTShadowsEnabled() || _vk.CloudShadowsActive() || ((_vk.ContactShadowsEligible() ||
                (_vk.SoftShadowsEligible() && _shadowTuning.enabled)) && !_waterSurface)) &&
                (!_softwareMesh->directSun.empty() || material) ?
                (_vk.RTShadowsEnabled() || _vk.SoftShadowsEligible() || _vk.CloudShadowsActive() ? 1.f : 2.f) : 0.f;
    }
    if (receiver.rtWater[0] && receiver.shadowReceiver[1] <= 0)
        receiver.rtWater = {}; // unsupported software projection retains ordinary water
    _vk.DrawMesh(mesh.buffers, 0, indices.size(), false, {}, {1, 1, 1, 1}, image,
                 vk::ShapeSampler(render::SplitLegacy(_softwareFlags)), cutoff, blend, true,
                 (_softwareFlags & NoZBuf) == 0, nullptr, waterBump, secondaryMode, {0, -1, 0}, mesh.vertexOffset, mesh.indexOffset,
                 (_softwareFlags & NoZWrite) == 0,
                 receiver.shadowReceiver[0] || receiver.atmosphereSun[3] || receiver.rtSun[0] || receiver.rtWater[0] ||
                         _temporalTerrain || localShadows || receiver.wetSurface[0] || receiver.rtWater[3] || receiver.aoReceiver[0] || material
                     ? &receiver
                     : nullptr,
                 shadow, !shadow && (_softwareFlags & IsLight) != 0);
}
void EngineVK::EndMesh(TLVertexTable& mesh)
{
    if (_softwareMesh != &mesh)
        throw std::logic_error("Vulkan software EndMesh must match BeginMesh");
    _softwareMesh = nullptr;
    _waterSurface = false;
    _softwareVertices.clear();
    _softwareMip = MipInfo();
}
} // namespace Poseidon
