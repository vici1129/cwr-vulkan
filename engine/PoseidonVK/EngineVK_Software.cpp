#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <PoseidonVK/ShapeLightingVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>

namespace Poseidon
{
void EngineVK::PrepareMesh(const render::LegacySpec&)
{
    // CPU transformation, lighting and clipping are performed by FaceArray.
    if (_softwareMesh || _activeShape)
        throw std::logic_error("Vulkan software mesh preparation requires an idle open frame");
}
void EngineVK::BeginMesh(TLVertexTable& mesh, const render::LegacySpec&)
{
    if (_temporalTerrain && !_waterSurface)
        CaptureTemporalTerrain(mesh, false);
    if (_softwareMesh || _activeShape)
        throw std::logic_error("Vulkan software BeginMesh requires an idle open frame");
    _softwareVertices.assign(mesh.NVertex(), {});
    _softwareMesh = &mesh;
}
void EngineVK::PrepareTriangle(const MipInfo& mip, int flags)
{
    const int allowed = NoZBuf | NoZWrite | IsAlpha | IsTransparent | ClampU | ClampV | NoClamp | PointSampling |
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
    SubmitSoftware(indices);
}
void EngineVK::SubmitSoftware(const std::vector<uint32_t>& indices)
{
    if (!_softwareMesh)
        throw std::logic_error("Vulkan software drawing requires BeginMesh");
    if (!_vk.FrameOpen())
        return; // A minimized drawable has no acquired frame; retain the engine mesh protocol.
    if (indices.empty())
        return;
    for (auto index : indices)
    {
        if (index >= _softwareVertices.size())
            throw std::out_of_range("Vulkan software face vertex exceeds the transformed table");
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
        _softwareVertices[index] = vk::ScreenGeometry(vertex, _width, _height);
        if (index < _softwareMesh->hdrColor.size() && _softwareMesh->hdrColor[index][3] != 0)
            std::copy_n(_softwareMesh->hdrColor[index].begin(), 3, _softwareVertices[index].color);
        if (index < _softwareMesh->previousClip.size())
            std::copy(_softwareMesh->previousClip[index].begin(), _softwareMesh->previousClip[index].end(),
                      _softwareVertices[index].previousClip);
        if (index < _softwareMesh->directSun.size())
            std::copy_n(_softwareMesh->directSun[index].begin(), 4, _softwareVertices[index].directSun);
        // TL already contains the scene Fog8 visibility in specular.a. Alpha
        // fog is already in color.a; it must not also get an RGB fog mix.
        _softwareVertices[index].fog =
            (_softwareFlags & (FogDisabled | NoDropdown | IsAlphaFog | IsLight)) ? 1.f : float(source.specular >> 24) / 255;
    }
    auto mesh = _vk.UploadTransientMesh(_softwareVertices.data(), _softwareVertices.size() * sizeof(vk::ScreenVertex),
                                        indices.data(), indices.size() * sizeof(uint32_t));
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
    vk::ShapeLighting receiver;
    std::shared_ptr<vk::TextureImage> waterBump;
    if (_waterSurface && _vk.WaterSurfaceEnabled())
    {
        try
        {
            auto texture = _textures.Load(Remaster >> "CfgDetailTextures" >> "specular");
            if (auto* bump = dynamic_cast<TextureVK*>(texture.GetRef()))
                waterBump = bump->Image(_vk);
            receiver.rtWater = {waterBump ? 1.f : 0.f, 0, 0, 0}; // shoreline UVs already animate in Landscape::DrawMesh
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
    if (_shadowWorld && (_shadowTuning.enabled || atmosphere || _vk.SeparateSunlight() || receiver.rtWater[0]) &&
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
                !_softwareMesh->directSun.empty() ?
                (_vk.RTShadowsEnabled() || _vk.SoftShadowsEligible() || _vk.CloudShadowsActive() ? 1.f : 2.f) : 0.f;
    }
    if (receiver.rtWater[0] && receiver.shadowReceiver[1] <= 0)
        receiver.rtWater = {}; // unsupported software projection retains ordinary water
    _vk.DrawMesh(mesh.buffers, 0, indices.size(), false, {}, {1, 1, 1, 1}, image,
                 vk::ShapeSampler(render::SplitLegacy(_softwareFlags)), cutoff, blend, true,
                 (_softwareFlags & NoZBuf) == 0, nullptr, waterBump, 0, {0, -1, 0}, mesh.vertexOffset, mesh.indexOffset,
                 (_softwareFlags & NoZWrite) == 0,
                 receiver.shadowReceiver[0] || receiver.atmosphereSun[3] || receiver.rtSun[0] || receiver.rtWater[0] ||
                         _temporalTerrain
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
