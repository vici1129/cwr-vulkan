#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <PoseidonVK/EnhancedMaterialMapsVK.hpp>
#include <PoseidonVK/TerrainRayVK.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <cmath>
#include <algorithm>

TEST_CASE("Terrain ray UVs preserve clamped cell seams and original bilinear jitter", "[Graphics][enhanced-material]")
{
    using Poseidon::vk::TerrainRayUV;
    const std::array<float, 4> u{0, .1f, .2f, .3f}, v{.1f, .2f, .3f, .4f};
    const auto a = TerrainRayUV(10.25f, 20.5f, 10, 20, true, u, v);
    REQUIRE(a[0] == Catch::Approx(.375f));
    REQUIRE(a[1] == Catch::Approx(.725f));
    const auto b = TerrainRayUV(10.25f, 20.5f, 10, 20, false, u, v);
    REQUIRE(b[0] - a[0] == Catch::Approx(10));
    REQUIRE(b[1] - a[1] == Catch::Approx(20));
    // Same physical boundary has U=1 on the left tile and U=0 on the right.
    const std::array<float, 4> zero{};
    REQUIRE(TerrainRayUV(11, 20, 10, 20, true, zero, zero)[0] == 1);
    REQUIRE(TerrainRayUV(11, 20, 11, 20, true, zero, zero)[0] == 0);
}

TEST_CASE("Terrain ray normals match the outward original raster stencil", "[Graphics][enhanced-material]")
{
    using Poseidon::vk::TerrainRayNormal;
    const auto flat = TerrainRayNormal(10, 10, 10, 10, 12.5f);
    REQUIRE(flat == std::array<float, 3>{0, 1, 0});
    const auto slope = TerrainRayNormal(8, 12, 9, 11, 12.5f);
    const float length = std::sqrt(16 + 4 + 12.5f * 12.5f);
    REQUIRE(slope[0] == Catch::Approx(-4 / length));
    REQUIRE(slope[1] == Catch::Approx(12.5f / length));
    REQUIRE(slope[2] == Catch::Approx(-2 / length));
    for (unsigned i = 0; i < std::size(Poseidon::vk::StockPBRMaterials); ++i)
        REQUIRE(Poseidon::vk::StockPBRIdentity(Poseidon::vk::StockPBRMaterials[i].texture) == 64 + i);
    REQUIRE(Poseidon::vk::StockPBRIdentity("data\\domek1_side.pac") == 0);
}

TEST_CASE("Stock enhancements do not require replacement artwork or data maps", "[Graphics][enhanced-material]")
{
    for (const auto* entry : Poseidon::vk::EnhancedCatalog)
    {
        const auto& material = *entry;
        REQUIRE(std::string_view(material.albedo).empty());
        REQUIRE(std::string_view(material.normal).empty());
        REQUIRE(std::string_view(material.roughnessMap).empty());
        REQUIRE(material.roughness == .9f);
        REQUIRE(material.metallic == 0);
    }
}

TEST_CASE("Independent material data maps pack without requiring either companion", "[Graphics][enhanced-material]")
{
    using namespace Poseidon;
    using vk::PackEnhancedMaps;
    const std::array normal{DecodedImage{{128, 200, 230, 0, 100, 128, 250, 0}, 2, 1}};
    const std::array roughness{DecodedImage{{190, 0, 0, 0}, 1, 1}};
    REQUIRE(PackEnhancedMaps({}, {}).empty());
    const auto n = PackEnhancedMaps(normal, {}), r = PackEnhancedMaps({}, roughness),
               both = PackEnhancedMaps(normal, roughness);
    REQUIRE(n[0].rgba == std::vector<uint8_t>{128, 200, 230, 255, 100, 128, 250, 255});
    REQUIRE(r[0].rgba == std::vector<uint8_t>{128, 128, 255, 190});
    REQUIRE(both[0].rgba == std::vector<uint8_t>{128, 200, 230, 190, 100, 128, 250, 190});
}

namespace EnhancedHistory
{
using std::abs;
using std::max;
using std::min;
#include <PoseidonVK/Shaders/rtgi_history.glsl>
} // namespace EnhancedHistory

TEST_CASE("Enhanced one-sample GI history remains bounded", "[Graphics][enhanced-material]")
{
    float length = 1;
    for (int i = 0; i < 64; ++i)
        length = EnhancedHistory::giHistoryLength(length, 16.f);
    REQUIRE(length == 16);
    REQUIRE(EnhancedHistory::giHistoryWeight(length) == Catch::Approx(15.f / 16));
    REQUIRE_FALSE(EnhancedHistory::giHistorySurface(10, 11, 1));
}

namespace MaterialNormalMath
{
// Compile the actual GLSL frame reconstruction with a small CPU vector adapter.
struct vec2
{
    float x, y;
};
struct vec3
{
    float x, y, z;
    vec3 operator*(float k) const { return {x * k, y * k, z * k}; }
    vec3 operator*(vec3 b) const { return {x * b.x, y * b.y, z * b.z}; }
    vec3 operator/(float k) const { return {x / k, y / k, z / k}; }
    vec3 operator+(vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    vec3 operator-(vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    void operator-=(vec3 b) { *this = *this - b; }
    void operator*=(float k) { *this = *this * k; }
};
float dot(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
vec3 cross(vec3 a, vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
vec3 normalize(vec3 a)
{
    return a / std::sqrt(dot(a, a));
}
using std::abs;
using std::max;
using std::min;
using std::sqrt;
using std::cos;
using std::sin;
using std::clamp;
vec3 clamp(vec3 c, float lo, float hi)
{
    return {std::clamp(c.x, lo, hi), std::clamp(c.y, lo, hi), std::clamp(c.z, lo, hi)};
}
#include <PoseidonVK/Shaders/material_normal.glsl>
#include <PoseidonVK/Shaders/material_diffuse.glsl>
#include <PoseidonVK/Shaders/diffuse_sky_math.glsl>
} // namespace MaterialNormalMath

TEST_CASE("Diffuse sky cosine sampling conserves Lambert energy and respects the horizon",
          "[Graphics][enhanced-material]")
{
    using namespace MaterialNormalMath;
    for (const vec3 n : {vec3{0,1,0},vec3{1,0,0},vec3{0,-1,0}})
    {
        float sky=0;
        for(int i=0;i<32;++i) for(int j=0;j<64;++j)
        {
            const auto d=giCosineDirection(n,(i+.5f)/32,(j+.5f)*6.2831853f/64);
            REQUIRE(dot(d,d)==Catch::Approx(1).margin(1e-5));
            REQUIRE(dot(d,n)>0);
            sky+=giSkyHemisphere(d,n)?1.f:0.f;
        }
        sky/=2048;
        REQUIRE(sky==Catch::Approx(n.y>0?1.f:n.y<0?0.f:.5f).margin(.001));
        const vec3 reflectance{.2f,.3f,.4f},sun{.4f,.2f,.1f};
        const auto out=giLambertOutgoing(reflectance,sun,{sky,sky,sky});
        REQUIRE(out.x==Catch::Approx(.2f*(.4f+sky)));
        REQUIRE(out.z==Catch::Approx(.4f*(.1f+sky)));
        // Blocking every sky ray removes sky, not sunlight, and adds no ambient.
        REQUIRE(giLambertOutgoing(reflectance,sun,{0,0,0}).x==Catch::Approx(.08f));
        REQUIRE(giLambertOutgoing(reflectance,{0,0,0},{0,0,0}).x==0);
    }
    REQUIRE_FALSE(giSkyHemisphere({0,-1,0},{0,-1,0}));
    REQUIRE_FALSE(giSkyHemisphere({0,1,0},{0,-1,0}));
    const auto horizon=giSkyGradient({.1f,.2f,.4f},{.3f,.3f,.3f},0);
    const auto zenith=giSkyGradient({.1f,.2f,.4f},{.3f,.3f,.3f},1);
    REQUIRE(horizon.x==.3f);
    REQUIRE(zenith.z==.4f);
    for(int i=0;i<4;++i)
    {
        REQUIRE(giSunComponent(float(i))==bool(i&1));
        REQUIRE(giSkyComponent(float(i))==bool(i&2));
    }
}

TEST_CASE("Sky irradiance calibration preserves chroma and geometric energy loss",
          "[Graphics][enhanced-material]")
{
    using namespace MaterialNormalMath;
    const vec3 dome{.27f,.46f,.86f}, reference{.75f,.91f,1.28f};
    const float scale=giSkyCalibration(reference,dome);
    const auto calibrated=dome*scale;
    REQUIRE(giSkyLuminance(calibrated)==Catch::Approx(giSkyLuminance(reference)));
    REQUIRE(calibrated.x/calibrated.z==Catch::Approx(dome.x/dome.z));
    REQUIRE(scale>1);
    REQUIRE(giSkyCalibration(dome,dome)==Catch::Approx(1));
    REQUIRE(giSkyCalibration(dome,reference)<1);
    REQUIRE(giSkyCalibration({0,0,0},dome)==0);
    REQUIRE(giSkyCalibration(reference,{0,0,0})==1);
    // Visibility is applied after calibration, never normalized away.
    REQUIRE(giSkyLuminance(calibrated*.25f)==Catch::Approx(giSkyLuminance(reference)*.25f));
    REQUIRE(giSkyLuminance(calibrated*0)==0);
    // A vertical surface sees half a uniform sky, not the full ambient reference.
    REQUIRE(giSkyLuminance(calibrated*.5f)==Catch::Approx(giSkyLuminance(reference)*.5f));
    const vec3 rho{.2f,.3f,.4f}, sun{.4f,.2f,.1f};
    const auto outgoing=giLambertOutgoing(rho,sun,calibrated);
    const auto withoutSun=giLambertOutgoing(rho,{0,0,0},calibrated);
    REQUIRE((outgoing-withoutSun).x==Catch::Approx(rho.x*sun.x));
    REQUIRE((outgoing-withoutSun).z==Catch::Approx(rho.z*sun.z));
}

TEST_CASE("Shared stock diffuse proxy bounds energy without inventing color or removing baked shading",
          "[Graphics][enhanced-material]")
{
    using namespace MaterialNormalMath;
    for (float v : {0.f, .05f, .2f, .5f, .8f, 1.f})
    {
        const vec3 source{v, v * .6f, v * .3f};
        const auto stock = materialBaseColor(source, 1), v3 = materialBaseColor(source, 0);
        REQUIRE(stock.x <= .5f);
        REQUIRE(stock.x <= source.x);
        REQUIRE(v3.x == v);
        REQUIRE(stock.y == Catch::Approx(stock.x * .6f));
        REQUIRE(stock.z == Catch::Approx(stock.x * .3f));
        REQUIRE(materialDiffuse(source, 1, .04f, 0).x == Catch::Approx(stock.x * .96f));
        REQUIRE(materialDiffuse(source, 1, .04f, 1).x == 0);
    }
}

TEST_CASE("Enhanced materials require an explicit model visual LOD and section", "[Graphics][enhanced-material]")
{
    using namespace Poseidon::vk;
    REQUIRE(FindEnhancedMaterial("Data3D/ZED_KAMENNA.p3d", .5f, "data\\kamennazed.pac", 48, 1) == &StoneWall);
    REQUIRE(FindEnhancedMaterial(StoneWall.model, 1, StoneWall.section, 24, 1) == &StoneWall);
    REQUIRE_FALSE(FindEnhancedMaterial("addon\\zed_kamenna.p3d", .5f, StoneWall.section, 48, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, 1e13f, StoneWall.section, 24, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, .5f, "addon\\wall.pac", 48, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, .5f, StoneWall.section, 49, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, .5f, StoneWall.section, 48, 2));
    REQUIRE(FindEnhancedMaterial(ConcretePanel.model, .5f, ConcretePanel.section, 68, 3)->id == ConcretePanel.id);
    REQUIRE(FindEnhancedMaterial(ConcretePillar.model, 4, ConcretePillar.section, 8, 2)->id == ConcretePillar.id);
    REQUIRE_FALSE(FindEnhancedMaterial(ConcreteCap.model, 4, ConcreteCap.section, 8, 2));
    REQUIRE_FALSE(FindEnhancedMaterial(ConcretePanel.model, .5f, "data\\zed_beton_mask_01_si.pac", 68, 3));
    for (const auto* material : EnhancedCatalog)
        for (float lod : {.5f, 1.f})
        {
            const auto geometry = EnhancedGeometryFingerprint(material->id, lod);
            REQUIRE(EnhancedSourceCompatible(*material, lod, geometry, material->width, material->height,
                                             material->textureFingerprint));
            REQUIRE_FALSE(EnhancedSourceCompatible(*material, lod, geometry ^ 1, material->width, material->height,
                                                   material->textureFingerprint));
            REQUIRE_FALSE(EnhancedSourceCompatible(*material, lod, geometry, material->width, material->height,
                                                   material->textureFingerprint ^ 1));
            REQUIRE_FALSE(EnhancedSourceCompatible(*material, lod, geometry, material->width + 1, material->height,
                                                   material->textureFingerprint));
        }
}

TEST_CASE("Material frames preserve mirrored UV orientation and degenerate fallback", "[Graphics][enhanced-material]")
{
    using namespace MaterialNormalMath;
    const vec3 n{0, 0, 1}, x{1, 0, 0}, y{0, 1, 0};
    const auto flat = materialNormal(n, x, y, {1, 0}, {0, 1}, n);
    REQUIRE(flat.z == Catch::Approx(1));
    const auto mirrored = materialNormal(n, x, y, {-1, 0}, {0, 1}, {.6f, 0, .8f});
    REQUIRE(mirrored.x == Catch::Approx(-.6f));
    REQUIRE(mirrored.z == Catch::Approx(.8f));
    const auto mirroredV = materialNormal(n, x, y, {1, 0}, {0, -1}, {0, .6f, .8f});
    REQUIRE(mirroredV.y == Catch::Approx(-.6f));
    REQUIRE(materialNormal(n, x, y, {0, 0}, {0, 0}, x).z == Catch::Approx(1));
    // Raster first reconstructs object derivatives under nonuniform scaling.
    // Ray hits already have object edges; both inverse-transpose the final n.
    const vec3 worldX{2, 0, 0}, worldY{0, .5f, 0};
    const vec3 objectX{worldX.x / 2, worldX.y / .5f, worldX.z / 3};
    const vec3 objectY{worldY.x / 2, worldY.y / .5f, worldY.z / 3};
    const auto raster = materialNormal(n, objectX, objectY, {1, 0}, {0, 1}, {.6f, 0, .8f});
    const auto ray = materialNormal(n, x, y, {1, 0}, {0, 1}, {.6f, 0, .8f});
    const auto transformed = normalize(vec3{raster.x / 2, raster.y / .5f, raster.z / 3});
    const auto expected = normalize(vec3{.3f, 0, .8f / 3});
    REQUIRE(dot(raster, ray) == Catch::Approx(1));
    REQUIRE(dot(transformed, expected) == Catch::Approx(1));
    REQUIRE(transformed.x > raster.x);
}

TEST_CASE("Ambient replacement preserves all other radiance and fog", "[Graphics][enhanced-material]")
{
    for (float ambient : {0.f, .1f, .5f})
        for (float direct : {0.f, .3f, 2.f})
            for (float local : {0.f, .7f})
                for (float emission : {0.f, .4f})
                    for (float fog : {0.f, .2f, 1.f})
                        for (float fade : {0.f, .5f, 1.f})
                        {
                            const float bounce = .12f, scatter = .2f * (1 - fog);
                            const float raster = (ambient + direct + local + emission) * fog + scatter;
                            const float delta = (bounce - ambient) * fog * fade;
                            const float expected =
                                (ambient * (1 - fade) + bounce * fade + direct + local + emission) * fog + scatter;
                            REQUIRE(raster + delta == Catch::Approx(expected).margin(1e-6));
                            REQUIRE(raster + delta >= 0);
                        }
}
