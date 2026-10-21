#pragma once
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <array>
#include <span>

namespace Poseidon::vk
{
// Additional source evidence for existing triangles, NOT a whole-atlas identity.
// Only triPBR auto consumes these rules. No generated maps or shader masks.
struct OriginalMaterialRegion
{
    unsigned identity, family;
    std::string_view model, texture;
    float roughness, uMin, vMin, uMax, vMax;
    int width, height;
    uint64_t pixels;
};
inline constexpr OriginalMaterialRegion OriginalMaterialRegions[]{
    {512,6,"data3d\\statek_kulna.p3d","data\\budova2_s.pac", .9f,0,.675f,1,1,256,128,10590596532554886688ull},
    {513,6,"data3d\\statek_kulna.p3d","data\\budova2_s2.pac",.9f,0,.625f,1,1,256,128,11073974456645178822ull},
    {514,6,"data3d\\statek_brana.p3d","data\\budova2_s.pac", .9f,0,.675f,1,1,256,128,10590596532554886688ull}};
struct OriginalRegionGeometry
{
    std::string_view model;
    float resolution;
    int vertices, sections;
    uint64_t fingerprint;
    uint64_t textureBindings=0;
};
inline constexpr OriginalRegionGeometry OriginalRegionGeometries[]{
    {"data3d\\statek_kulna.p3d",.5f,83,6,5710411994966935590ull,9466661084647219132ull},
    {"data3d\\statek_kulna.p3d",1.f,48,6,4372031775053796802ull,1700184925328476168ull},
    {"data3d\\statek_kulna.p3d",3.f,41,5,4806727032941264610ull,1282800507099886727ull},
    {"data3d\\statek_brana.p3d",.5f,198,5,8877832809354361115ull,6883824423279479731ull},
    {"data3d\\statek_brana.p3d",1.f,114,5,1080067108508164804ull,17298411900941076431ull},
    {"data3d\\statek_brana.p3d",1.5f,58,3,8086190560358923383ull,16878692861437147427ull}};
inline const OriginalMaterialRegion* OriginalRegion(unsigned identity)
{
    for (const auto& r : OriginalMaterialRegions) if (r.identity==identity) return &r;
    return nullptr;
}
inline unsigned OriginalRegionCandidate(std::string_view model, float lod, int vertices, int sections,
                                        uint64_t fingerprint, std::string_view texture)
{
    // TextureVK supplies its canonical bank key. Fail cheaply for every other
    // source before allocating a model key in the per-frame ray submission path.
    if (!std::any_of(std::begin(OriginalMaterialRegions),std::end(OriginalMaterialRegions),
        [&](const auto& r){return r.texture==texture;})) return 0;
    const auto key=MaterialIdentity(model);
    if (!std::any_of(std::begin(OriginalRegionGeometries),std::end(OriginalRegionGeometries),
        [&](const auto& g) { return g.model==key && g.resolution==lod && g.vertices==vertices &&
                                   g.sections==sections && g.fingerprint==fingerprint; })) return 0;
    for (const auto& r : OriginalMaterialRegions)
        if (r.model==key && r.texture==texture) return r.identity;
    return 0;
}
inline uint64_t OriginalRegionTextureBindings(std::span<const std::string_view> textures)
{
    uint64_t hash=1469598103934665603ull;
    for (const auto name : textures)
    {
        for (unsigned char c : MaterialIdentity(name)) hash=(hash^c)*1099511628211ull;
        hash=(hash^0)*1099511628211ull; // distinguish adjacent resource keys and null entries
    }
    return hash;
}
inline bool OriginalRegionBindingsCompatible(uint64_t geometry, uint64_t textures)
{
    return std::any_of(std::begin(OriginalRegionGeometries),std::end(OriginalRegionGeometries),
        [&](const auto& g){return g.fingerprint==geometry && g.textureBindings && g.textureBindings==textures;});
}
inline bool OriginalRegionUV(unsigned identity, const std::array<std::array<float,2>,3>& triangle)
{
    const auto* region=OriginalRegion(identity);
    if (!region) return false;
    // A convex rectangle contains the ENTIRE original triangle when it contains
    // every corner. No wrapping, clipping a mixed triangle, or edge extrapolation.
    return std::all_of(triangle.begin(),triangle.end(),[&](const auto& uv) {
        return std::isfinite(uv[0]) && std::isfinite(uv[1]) &&
            uv[0]>=region->uMin && uv[0]<=region->uMax && uv[1]>=region->vMin && uv[1]<=region->vMax;
    });
}
inline uint64_t OriginalRegionPixelFingerprint(std::span<const uint8_t> pixels)
{
    uint64_t hash=1469598103934665603ull;
    for (auto byte : pixels) hash=(hash^byte)*1099511628211ull;
    return hash;
}
inline QualifiedMaterial OriginalRegionSource(unsigned identity, std::string_view texture,
                                              int width, int height, uint64_t pixels)
{
    const auto* r=OriginalRegion(identity);
    return r && texture==r->texture && width==r->width && height==r->height && pixels==r->pixels ?
        QualifiedMaterial{r->identity,r->family,r->roughness} : QualifiedMaterial{};
}
} // namespace Poseidon::vk
