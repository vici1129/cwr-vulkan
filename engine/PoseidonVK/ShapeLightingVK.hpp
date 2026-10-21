#pragma once
#include <PoseidonVK/ShapeTransformVK.hpp>
#include <PoseidonVK/ShapeLightingData.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <PoseidonVK/MaterialQualificationVK.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>

namespace Poseidon::vk
{
// Call only after stock/catalog material setup. The caller's opaque-world gate
// and captured PBR material remain prerequisites, not consequences of identity.
inline void QualifyGIReceiver(ShapeLighting& lighting, bool software)
{
    if (!(lighting.pbrDiffuse[3] >= 1 && lighting.pbrDiffuse[3] <= 7) ||
        !(lighting.pbr[3] >= 1 && (lighting.pbr[3] < 128 + std::size(EnhancedPBRMaterials) ||
                                 lighting.pbr[3] == SourceDiffuseIdentity ||
                                 std::any_of(std::begin(OriginalMaterialRegions),std::end(OriginalMaterialRegions),
                                     [&](const auto& r){return lighting.pbr[3]==float(r.identity);})))) return;
    const int family = int(lighting.pbrDiffuse[3]);
    const unsigned id = unsigned(lighting.pbr[3]);
    if (float(family) != lighting.pbrDiffuse[3] || float(id) != lighting.pbr[3]) return;
    const bool stock = family == int(PBRReceiverFamily(id)) && family > 0;
    const bool catalog = family == 1 && std::any_of(EnhancedCatalog.begin(), EnhancedCatalog.end(),
        [&](const auto* material) { return material->id == id; });
    const bool source = family == 1 && id == SourceDiffuseIdentity;
    const auto* region=OriginalRegion(id);
    const bool regional=region && region->reception && family==int(region->family);
    if (lighting.rtWater[3] == 1 && lighting.pbr[0] > 0 && (stock || catalog || source || regional))
    {
        lighting.rtWater[3] = 2;
        lighting.pbrDiffuse[3] = float(family + (software ? 8 : 0));
    }
}
inline Vector3 SoftwarePBRNormal(const Matrix4& worldToModel, Vector3Par n)
{
    return Vector3(
        -(worldToModel(0,0)*n.X()+worldToModel(1,0)*n.Y()+worldToModel(2,0)*n.Z()),
        -(worldToModel(0,1)*n.X()+worldToModel(1,1)*n.Y()+worldToModel(2,1)*n.Z()),
        -(worldToModel(0,2)*n.X()+worldToModel(1,2)*n.Y()+worldToModel(2,2)*n.Z()));
}
inline void ShapeWorld(ShapeLighting& out, Matrix4Val relativeWorld)
{
    GfxMatrix matrix{};
    ConvertMatrix(matrix, relativeWorld);
    std::copy_n(&matrix._11, 16, out.world.begin());
    const auto inverse = relativeWorld.Orientation().InverseGeneral();
    for (int col = 0; col < 3; ++col)
        for (int row = 0; row < 3; ++row)
            out.normal[col * 4 + row] = inverse(col, row);
}
inline void ShapeMaterial(ShapeLighting& out, const TLMaterial& material, const LightSun& sun, bool enabled)
{
    const auto ambient = sun.Ambient() * material.ambient + sun.Diffuse() * material.forcedDiffuse;
    const auto diffuse = sun.Diffuse() * material.diffuse;
    out.ambient = {ambient.R(), ambient.G(), ambient.B(), enabled ? 1.f : 0.f};
    out.diffuse = {diffuse.R(), diffuse.G(), diffuse.B(), 0};
    const auto forced = sun.Diffuse() * material.forcedDiffuse;
    out.rtSun[1] = forced.R();
    out.rtSun[2] = forced.G();
    out.rtSun[3] = forced.B();
    out.emissive = {material.emmisive.R(), material.emmisive.G(), material.emmisive.B(), 0};
    const auto specular = sun.Diffuse() * material.specular;
    out.specular = {specular.R(), specular.G(), specular.B(), float(material.specularPower)};
    const auto direction = sun.Direction();
    out.sunDirection = {direction.X(), direction.Y(), direction.Z(), 0};
    out.pbrDiffuse = {material.diffuse.R(), material.diffuse.G(), material.diffuse.B(), 0};
}
inline void ShapeFog(ShapeLighting& out, float start, float end, ColorVal color, bool enabled)
{
    out.fogParams = {start, end > start ? 1.f / (end - start) : 0, enabled ? 1.f : 0.f, 0};
    out.fogColor = {color.R(), color.G(), color.B(), 1};
}
inline void ShapeLight(ShapeLocalLight& out, const LightDescription& light, const TLMaterial& material,
                       Vector3Par camera, float night)
{
    const auto position = light.pos - camera;
    const auto direction = light.dir.Normalized();
    const auto diffuse = light.diffuse * material.diffuse * night;
    const auto ambient = light.ambient * material.ambient * night;
    out.position = {position.X(), position.Y(), position.Z(), light.startAtten};
    out.direction = {direction.X(), direction.Y(), direction.Z(), light.type == LTSpotLight ? 1.f : 0.f};
    out.diffuse = {diffuse.R(), diffuse.G(), diffuse.B(), 0};
    out.ambient = {ambient.R(), ambient.G(), ambient.B(), 0};
}
} // namespace Poseidon::vk
